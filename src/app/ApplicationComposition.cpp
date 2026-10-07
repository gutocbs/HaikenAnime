#include "ApplicationComposition.h"

#include "InitialSyncCoordinator.h"
#include "ApplicationSyncTaskExecutor.h"

#include "../infrastructure/database/SqlQueryStore.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "../infrastructure/database/SqliteMediaRepository.h"
#include "../infrastructure/database/SqlitePendingChangeRepository.h"
#include "../infrastructure/database/SqliteQueryConfiguration.h"
#include "../infrastructure/configuration/JsonSettingsReader.h"
#include "../infrastructure/database/SqliteCoverCacheRepository.h"
#include "../infrastructure/database/SqliteUserPreferencesRepository.h"
#include "../infrastructure/covers/CoverFileStore.h"
#include "../infrastructure/covers/CoverTemporaryStore.h"
#include "../infrastructure/covers/QtCoverDownloader.h"
#include "../infrastructure/library/LocalLibraryScanner.h"
#include "../infrastructure/library/QtLocalFileOpener.h"
#include "../infrastructure/library/QtDirectoryEnumerator.h"
#include "../infrastructure/database/SqliteLocalFileRepository.h"
#include "../infrastructure/database/SqliteSyncTaskStateRepository.h"
#include "../infrastructure/anilist/HttpFactory.h"
#include <QStandardPaths>
#include <QDir>
#include <QPointer>
#include <QTimer>

namespace {
QString initializationFailure(const QString &stage, const QString &detail) {
    if (detail.isEmpty()) {
        return QStringLiteral("Application initialization failed while %1.").arg(stage);
    }
    return QStringLiteral("Application initialization failed while %1: %2").arg(stage, detail);
}

// Keep borrowed dependencies inside the worker-owned factory products. Member
// order destroys the scanner/repository before its enumerator/connection owner.
class OwnedLibraryScanner final : public ILocalLibraryScanner {
public:
    OwnedLibraryScanner() : scanner_(enumerator_) {}
    LocalLibraryScanResult scan(const LocalLibraryScanRequest &request,
        const std::function<bool(const QList<LocalFileObservation> &, QString &)> &consumer,
        const std::function<void(const LocalLibraryScanProgress &)> &progress,
        const std::function<bool()> &stop) override {
        return scanner_.scan(request, consumer, progress, stop);
    }
private:
    QtDirectoryEnumerator enumerator_;
    LocalLibraryScanner scanner_;
};

class OwnedLocalFileRepository final : public ILocalFileRepository {
public:
    OwnedLocalFileRepository(std::unique_ptr<SqliteDatabase> database, const QStringList &queries)
        : database_(std::move(database)), repository_(database_->connection(),
          queries[0], queries[1], queries[2], queries[3], queries[4],
          queries.size() > 5 ? queries[5] : QString{},
          queries.size() > 6 ? queries[6] : QString{},
          queries.size() > 7 ? queries[7] : QString{}) {}
    bool beginScan(const QString &root, qint64 &id, QString &error) override {
        return repository_.beginScan(root, id, error);
    }
    bool upsertBatch(qint64 id, const QList<LocalFileObservation> &files, QString &error) override {
        return repository_.upsertBatch(id, files, error);
    }
    bool completeScan(qint64 id, qsizetype count, QString &error) override {
        return repository_.completeScan(id, count, error);
    }
    bool failScan(qint64 id, LibraryScanStatus status, qsizetype count,
                  const QString &diagnostic, QString &error) override {
        return repository_.failScan(id, status, count, diagnostic, error);
    }
    bool readPendingRecognition(const QString &root, QList<LocalFileRecognitionRecord> &records,
                               QString &error) override {
        return repository_.readPendingRecognition(root, records, error);
    }
    bool readRecognitionCatalog(QList<Media> &media, QString &error) override {
        return repository_.readRecognitionCatalog(media, error);
    }
    bool saveRecognitionBatch(const QList<LocalFileRecognitionRecord> &records, QString &error) override {
        return repository_.saveRecognitionBatch(records, error);
    }
private:
    std::unique_ptr<SqliteDatabase> database_;
    SqliteLocalFileRepository repository_;
};

class OwnedSyncTaskStateRepository final : public ISyncTaskStateRepository {
public:
    OwnedSyncTaskStateRepository(std::unique_ptr<SqliteDatabase> database, QString readQuery,
                                 QString upsertQuery, QString deleteQuery)
        : database_(std::move(database)), repository_(database_->connection(), std::move(readQuery),
          std::move(upsertQuery), std::move(deleteQuery)) {}
    bool ReadAll(QList<SyncTaskState> &states, QString &error) override { return repository_.ReadAll(states, error); }
    bool Upsert(const SyncTaskState &state, QString &error) override { return repository_.Upsert(state, error); }
    bool Remove(const SyncPartition &partition, QString &error) override { return repository_.Remove(partition, error); }
private:
    std::unique_ptr<SqliteDatabase> database_;
    SqliteSyncTaskStateRepository repository_;
};

bool hasExistingQueries(const SqliteQueryConfiguration &queries) {
    return !queries.upsertMediaPath.isEmpty() && !queries.updatePersonalListMediaPath.isEmpty()
        && !queries.readMediaPath.isEmpty()
        && !queries.readActiveMediaIdsPath.isEmpty() && !queries.markMediaSourceRemovedPath.isEmpty()
        && !queries.enqueuePendingChangePath.isEmpty() && !queries.readPendingChangesPath.isEmpty()
        && !queries.updatePendingChangePath.isEmpty() && !queries.readCoverCachePath.isEmpty()
        && !queries.upsertCoverCachePath.isEmpty() && !queries.deleteCoverCachePath.isEmpty()
        && !queries.clearCoverCachePath.isEmpty() && !queries.readUserPreferencesPath.isEmpty()
        && !queries.upsertUserPreferencesPath.isEmpty();
}

void composeLibraryScanner(ApplicationContext &context, const SqliteQueryConfiguration &configuration) {
    QStringList queries;
    QString scannerError;
    for (const auto &path : {configuration.beginLibraryScanPath, configuration.upsertLocalFilePath,
         configuration.completeLibraryScanPath, configuration.failLibraryScanPath,
         configuration.markLocalFilesUnavailablePath, configuration.readPendingLocalFilesPath,
         configuration.readCatalogMediaForRecognitionPath, configuration.saveLocalFileRecognitionPath}) {
        if (path.isEmpty()) {
            scannerError = QStringLiteral("Local library scan query configuration is incomplete.");
            break;
        }
        queries.append(path);
    }
    if (!scannerError.isEmpty()) context.logger->warning(LogCategory::QueryConfiguration, scannerError);
    const auto databasePath = context.database->databasePath();
    context.localLibraryScan = std::make_unique<LocalLibraryScanCoordinator>(
        [](QString &) -> std::unique_ptr<ILocalLibraryScanner> {
            return std::make_unique<OwnedLibraryScanner>();
        },
        [databasePath, queries, scannerError](QString &error) -> std::unique_ptr<ILocalFileRepository> {
            if (!scannerError.isEmpty()) { error = scannerError; return {}; }
            auto database = std::make_unique<SqliteDatabase>(databasePath);
            if (!database->open()) { error = database->lastError(); return {}; }
            return std::make_unique<OwnedLocalFileRepository>(std::move(database), queries);
        });
    context.localLibraryScan->setLogger(context.logger.get());
}

void composeLibraryRecognition(ApplicationContext &context,
                               const SqliteQueryConfiguration &configuration) {
    QStringList queries;
    QString recognitionError;
    for (const auto &path : {configuration.beginLibraryScanPath, configuration.upsertLocalFilePath,
         configuration.completeLibraryScanPath, configuration.failLibraryScanPath,
         configuration.markLocalFilesUnavailablePath, configuration.readPendingLocalFilesPath,
         configuration.readCatalogMediaForRecognitionPath, configuration.saveLocalFileRecognitionPath}) {
        if (path.isEmpty()) {
            recognitionError = QStringLiteral("Local library recognition query configuration is incomplete.");
            break;
        }
        queries.append(path);
    }
    if (!recognitionError.isEmpty()) {
        context.logger->warning(LogCategory::QueryConfiguration, recognitionError);
    }
    const auto databasePath = context.database->databasePath();
    context.localLibraryRecognition = std::make_unique<LocalLibraryRecognitionCoordinator>(
        [databasePath, queries, recognitionError](QString &error) -> std::unique_ptr<ILocalFileRepository> {
            if (!recognitionError.isEmpty()) {
                error = recognitionError;
                return {};
            }
            auto database = std::make_unique<SqliteDatabase>(databasePath);
            if (!database->open()) {
                error = database->lastError();
                return {};
            }
            return std::make_unique<OwnedLocalFileRepository>(std::move(database), queries);
        });
    context.localLibraryRecognition->setLogger(context.logger.get());
    QObject::connect(context.localLibraryScan.get(), &LocalLibraryScanCoordinator::succeeded,
                     context.localLibraryRecognition.get(),
                     [recognition = context.localLibraryRecognition.get()](const QString &rootPath, qsizetype) {
                         recognition->start(rootPath);
                     });
}

void composeLocalEpisodeServices(ApplicationContext &context,
                                 const SqliteQueryConfiguration &configuration) {
    QString error;
    if (configuration.readNextLocalEpisodePath.isEmpty()
        || configuration.readAvailableEpisodeCountPath.isEmpty()) {
        error = QStringLiteral("Local episode query configuration is incomplete.");
    }
    if (!error.isEmpty()) {
        context.logger->warning(LogCategory::QueryConfiguration, error);
        return;
    }
    context.localEpisodeReader = std::make_unique<LocalEpisodeReader>(
        context.database->connection(), configuration.readNextLocalEpisodePath,
        configuration.readAvailableEpisodeCountPath);
    context.localFileOpener = std::make_unique<QtLocalFileOpener>();
}
}

ApplicationContext createApplicationContext() {
    return createApplicationContext(ApplicationCompositionOptions{});
}

ApplicationContext createApplicationContext(const ApplicationCompositionOptions &options) {
    ApplicationContext context;
    Settings settings;
    QString settingsError;
    // Settings is packaged as a Qt resource so startup does not depend on the
    // process working directory chosen by the IDE, service manager or shell.
    JsonSettingsReader settingsReader(options.settingsPath);
    if (!settingsReader.read(settings, settingsError)) {
        settings.logRetentionDays = 7;
    }
    context.logger = std::make_unique<AsyncLogger>(settings.logRetentionDays);
    context.logger->start();
    context.logger->info(LogCategory::Application, QStringLiteral("Starting application composition."));
    if (!settingsError.isEmpty()) {
        context.logger->warning(LogCategory::Configuration, settingsError);
    }
    context.database = std::make_unique<SqliteDatabase>(options.databasePath);
    context.database->setLogger(context.logger.get());

    if (!context.database->open()) {
        context.logger->error(LogCategory::Database, context.database->lastError());
        context.initializationError = initializationFailure(
            QStringLiteral("opening the SQLite database"), context.database->lastError());
        context.database.reset();
        return context;
    }

    if (!context.database->migrate()) {
        context.logger->error(LogCategory::Migration, context.database->lastError());
        context.initializationError = initializationFailure(
            QStringLiteral("migrating the SQLite database"), context.database->lastError());
        context.database.reset();
        return context;
    }

    QString queryError;
    SqliteQueryConfiguration queryConfiguration;
    queryConfiguration.setLogger(context.logger.get());
    if (!queryConfiguration.load(queryError, options.queryConfigurationPath)
        && !hasExistingQueries(queryConfiguration)) {
        context.logger->error(LogCategory::QueryConfiguration, queryError);
        context.initializationError = initializationFailure(
            QStringLiteral("loading the SQLite query configuration"), queryError);
        context.database.reset();
        return context;
    }
    auto mediaRepository = std::make_unique<SqliteMediaRepository>(
        context.database->connection(), queryConfiguration.upsertMediaPath,
        queryConfiguration.readMediaPath, queryConfiguration.readActiveMediaIdsPath,
        queryConfiguration.markMediaSourceRemovedPath,
        queryConfiguration.updatePersonalListMediaPath);
    mediaRepository->setLogger(context.logger.get());
    context.mediaRepository = std::move(mediaRepository);
    context.pendingChangeRepository = std::make_unique<SqlitePendingChangeRepository>(
        context.database->connection(), queryConfiguration.enqueuePendingChangePath,
        queryConfiguration.readPendingChangesPath, queryConfiguration.updatePendingChangePath);

    composeLibraryScanner(context, queryConfiguration);
    composeLibraryRecognition(context, queryConfiguration);
    composeLocalEpisodeServices(context, queryConfiguration);

    {
        auto preferencesRepository = std::make_unique<SqliteUserPreferencesRepository>(
            context.database->connection(), queryConfiguration.readUserPreferencesPath,
            queryConfiguration.upsertUserPreferencesPath);
        bool found = false;
        QString preferencesError;
        QString preferencesWarning;
        if (!preferencesRepository->read(context.userPreferences, found, preferencesError,
                                         preferencesWarning)) {
            context.userPreferences = settings.userPreferences;
            context.logger->warning(LogCategory::Configuration, preferencesError);
        } else if (!found) {
            context.userPreferences = settings.userPreferences;
            if (!preferencesRepository->replace(context.userPreferences, preferencesError)) {
                context.logger->warning(LogCategory::Configuration, preferencesError);
            }
        }
        if (!preferencesWarning.isEmpty()) {
            context.logger->warning(LogCategory::Configuration, preferencesWarning);
        }
        context.userPreferencesRepository = std::move(preferencesRepository);
    }

    context.coverCacheRepository = std::make_unique<SqliteCoverCacheRepository>(
        context.database->connection(), queryConfiguration.readCoverCachePath,
        queryConfiguration.upsertCoverCachePath, queryConfiguration.deleteCoverCachePath,
        queryConfiguration.clearCoverCachePath);
    const QString cacheRoot = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath("covers");
    const QString temporaryRoot = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath("HaikenAnime/covers");
    context.coverTemporaryStore = std::make_unique<CoverTemporaryStore>(temporaryRoot);
    int removedTemporaryFiles = 0;
    QString temporaryCleanupError;
    if (!context.coverTemporaryStore->ClearAbandoned(removedTemporaryFiles, temporaryCleanupError)) {
        context.logger->warning(LogCategory::Covers, temporaryCleanupError);
    } else if (removedTemporaryFiles > 0) {
        context.logger->info(LogCategory::Covers,
            QStringLiteral("Removed %1 abandoned cover download(s).").arg(removedTemporaryFiles));
    }
    context.coverFileStore = std::make_unique<CoverFileStore>(cacheRoot, settings.covers);
    context.coverDownloader = std::make_unique<QtCoverDownloader>(temporaryRoot, settings.covers);
    context.coverCoordinator = std::make_unique<CoverDownloadCoordinator>(
        *context.coverDownloader, *context.coverCacheRepository, *context.coverFileStore, settings.covers);
    context.coverCoordinator->setLogger(context.logger.get());
    context.clearLocalCache = std::make_unique<ClearLocalCacheUseCase>(
        *context.coverCoordinator, *context.coverTemporaryStore,
        QList<ICacheCleanupParticipant *>{});
    context.coverQuality = settings.covers.quality;
    context.initialSync = std::make_unique<InitialSyncCoordinator>(
        context.database->databasePath(),
        QStringLiteral(":/fixtures/graphql/userlist-response.json"),
        queryConfiguration.upsertMediaPath, queryConfiguration.readMediaPath,
        queryConfiguration.readActiveMediaIdsPath,
        queryConfiguration.markMediaSourceRemovedPath,
        settings.syncTimeoutMs, settings.syncIntervalMs);
    context.initialSync->setLogger(context.logger.get());
    const auto synchronizationDatabasePath = context.database->databasePath();
    const auto readTaskStatesQueryPath = queryConfiguration.readSyncTaskStatesPath;
    const auto upsertTaskStateQueryPath = queryConfiguration.upsertSyncTaskStatePath;
    const auto deleteTaskStateQueryPath = queryConfiguration.deleteSyncTaskStatePath;
    context.adaptiveSync = std::make_unique<AdaptiveSyncRuntime>(
        [synchronizationDatabasePath, readTaskStatesQueryPath, upsertTaskStateQueryPath,
         deleteTaskStateQueryPath](QString &error) -> std::unique_ptr<ISyncTaskStateRepository> {
            auto database = std::make_unique<SqliteDatabase>(synchronizationDatabasePath);
            if (!database->open() || !database->migrate()) {
                error = database->lastError();
                return {};
            }
            return std::make_unique<OwnedSyncTaskStateRepository>(std::move(database), readTaskStatesQueryPath,
                                                                   upsertTaskStateQueryPath, deleteTaskStateQueryPath);
        },
        [synchronizationDatabasePath, fixturePath = QStringLiteral(":/fixtures/graphql/userlist-response.json"),
         upsertMediaPath = queryConfiguration.upsertMediaPath, readMediaPath = queryConfiguration.readMediaPath,
         readActiveMediaIdsPath = queryConfiguration.readActiveMediaIdsPath,
         markSourceRemovedPath = queryConfiguration.markMediaSourceRemovedPath, readTaskStatesQueryPath,
         upsertTaskStateQueryPath, deleteTaskStateQueryPath, timeoutMs = settings.syncTimeoutMs] {
            return std::make_unique<ApplicationSyncTaskExecutor>(
                synchronizationDatabasePath, fixturePath, upsertMediaPath, readMediaPath, readActiveMediaIdsPath,
                markSourceRemovedPath, readTaskStatesQueryPath, upsertTaskStateQueryPath, deleteTaskStateQueryPath,
                timeoutMs);
        }, settings.syncTaskPolicies);
    context.seasonalNetworkManager.reset(HttpFactory::createNetworkAccessManager(nullptr));
    context.seasonalGraphQlClient = std::make_unique<AniListGraphQlClient>(
        *context.seasonalNetworkManager);
    context.seasonalQueryStore = std::make_unique<GraphQlQueryStore>(
        QStringLiteral(":/anilist/queries/seasonal-catalog.graphql"));
    context.seasonalCatalogDataSource = std::make_unique<GraphQlSeasonalCatalogDataSource>(
        *context.seasonalGraphQlClient, *context.seasonalQueryStore);
    context.seasonalCatalogCoordinator = std::make_unique<SeasonalCatalogCoordinator>(
        *context.seasonalCatalogDataSource, kSeasonalCatalogMaximumPageSize, nullptr,
        settings.seasonalCatalogCachePolicy);
    context.seasonalCatalogCoordinator->setLogger(context.logger.get());
    context.logger->info(LogCategory::Application, QStringLiteral("Application composition completed."));
    return context;
}

bool scheduleStartupLibraryScan(ApplicationContext &context, QObject *lifetime) {
    if (!lifetime || !context.localLibraryScan || context.startupLibraryScanScheduled
        || !context.userPreferences.automaticLocalFileRecognition) return false;
    context.startupLibraryScanScheduled = true;
    LocalLibraryScanRequest request;
    request.rootPath = context.userPreferences.libraryRoot;
    request.allowedExtensions = context.userPreferences.scanExtensions;
    const QPointer<LocalLibraryScanCoordinator> coordinator(context.localLibraryScan.get());
    QTimer::singleShot(0, lifetime, [coordinator, request] {
        if (coordinator) coordinator->start(request);
    });
    return true;
}
