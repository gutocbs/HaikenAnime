#include "ApplicationComposition.h"

#include "InitialSyncCoordinator.h"

#include "../infrastructure/database/SqlQueryStore.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "../infrastructure/database/SqliteMediaRepository.h"
#include "../infrastructure/database/SqlitePendingChangeRepository.h"
#include "../infrastructure/database/SqliteQueryConfiguration.h"
#include "../infrastructure/configuration/JsonSettingsReader.h"
#include "../infrastructure/database/SqliteCoverCacheRepository.h"
#include "../infrastructure/database/SqliteUserPreferencesRepository.h"
#include "../infrastructure/covers/CoverFileStore.h"
#include "../infrastructure/covers/QtCoverDownloader.h"
#include "../infrastructure/library/LocalLibraryScanner.h"
#include "../infrastructure/library/QtDirectoryEnumerator.h"
#include "../infrastructure/database/SqliteLocalFileRepository.h"
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
          queries[0], queries[1], queries[2], queries[3], queries[4]) {}
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
private:
    std::unique_ptr<SqliteDatabase> database_;
    SqliteLocalFileRepository repository_;
};

bool hasExistingQueries(const SqliteQueryConfiguration &queries) {
    return !queries.upsertMediaPath.isEmpty() && !queries.readMediaPath.isEmpty()
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
         configuration.markLocalFilesUnavailablePath}) {
        QString query;
        if (path.isEmpty()) {
            scannerError = QStringLiteral("Local library scan query configuration is incomplete.");
            break;
        }
        SqlQueryStore store(path);
        if (!store.load(query, scannerError)) break;
        queries.append(query);
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

    QString upsertQuery;
    QString readQuery;
    QString readActiveMediaIdsQuery;
    QString markSourceRemovedQuery;
    QString enqueuePendingQuery;
    QString readPendingQuery;
    QString updatePendingQuery;
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
    SqlQueryStore upsertStore(queryConfiguration.upsertMediaPath);
    SqlQueryStore readStore(queryConfiguration.readMediaPath);
    SqlQueryStore readActiveMediaIdsStore(queryConfiguration.readActiveMediaIdsPath);
    SqlQueryStore markSourceRemovedStore(queryConfiguration.markMediaSourceRemovedPath);
    upsertStore.setLogger(context.logger.get());
    readStore.setLogger(context.logger.get());
    if (!upsertStore.load(upsertQuery, queryError)
        || !readStore.load(readQuery, queryError)
        || !readActiveMediaIdsStore.load(readActiveMediaIdsQuery, queryError)
        || !markSourceRemovedStore.load(markSourceRemovedQuery, queryError)) {
        context.logger->error(LogCategory::QueryStore, queryError);
        context.initializationError = initializationFailure(
            QStringLiteral("loading the configured media queries"), queryError);
        context.database.reset();
        return context;
    }

    SqlQueryStore enqueueStore(queryConfiguration.enqueuePendingChangePath);
    SqlQueryStore readPendingStore(queryConfiguration.readPendingChangesPath);
    SqlQueryStore updatePendingStore(queryConfiguration.updatePendingChangePath);
    if (!enqueueStore.load(enqueuePendingQuery, queryError)
        || !readPendingStore.load(readPendingQuery, queryError)
        || !updatePendingStore.load(updatePendingQuery, queryError)) {
        context.initializationError = initializationFailure(
            QStringLiteral("loading the pending-change queries"), queryError);
        context.database.reset();
        return context;
    }

    auto mediaRepository = std::make_unique<SqliteMediaRepository>(
        context.database->connection(), std::move(upsertQuery), std::move(readQuery),
        std::move(readActiveMediaIdsQuery), std::move(markSourceRemovedQuery));
    mediaRepository->setLogger(context.logger.get());
    context.mediaRepository = std::move(mediaRepository);
    context.pendingChangeRepository = std::make_unique<SqlitePendingChangeRepository>(
        context.database->connection(), std::move(enqueuePendingQuery),
        std::move(readPendingQuery), std::move(updatePendingQuery));

    composeLibraryScanner(context, queryConfiguration);

    QString readPreferencesQuery;
    QString upsertPreferencesQuery;
    SqlQueryStore readPreferencesStore(queryConfiguration.readUserPreferencesPath);
    SqlQueryStore upsertPreferencesStore(queryConfiguration.upsertUserPreferencesPath);
    if (readPreferencesStore.load(readPreferencesQuery, queryError)
        && upsertPreferencesStore.load(upsertPreferencesQuery, queryError)) {
        context.userPreferencesRepository = std::make_unique<SqliteUserPreferencesRepository>(
            context.database->connection(), std::move(readPreferencesQuery),
            std::move(upsertPreferencesQuery));
        bool found = false;
        QString preferencesError;
        if (!context.userPreferencesRepository->read(context.userPreferences, found, preferencesError)) {
            context.userPreferences = settings.userPreferences;
            context.logger->warning(LogCategory::Configuration, preferencesError);
        } else if (!found) {
            context.userPreferences = settings.userPreferences;
            if (!context.userPreferencesRepository->replace(context.userPreferences, preferencesError)) {
                context.logger->warning(LogCategory::Configuration, preferencesError);
            }
        }
    } else {
        context.userPreferences = settings.userPreferences;
        context.logger->warning(LogCategory::Configuration, queryError);
    }

    QString readCoverQuery, upsertCoverQuery, deleteCoverQuery, clearCoverQuery;
    SqlQueryStore readCoverStore(queryConfiguration.readCoverCachePath);
    SqlQueryStore upsertCoverStore(queryConfiguration.upsertCoverCachePath);
    SqlQueryStore deleteCoverStore(queryConfiguration.deleteCoverCachePath);
    SqlQueryStore clearCoverStore(queryConfiguration.clearCoverCachePath);
    if (!readCoverStore.load(readCoverQuery, queryError)
        || !upsertCoverStore.load(upsertCoverQuery, queryError)
        || !deleteCoverStore.load(deleteCoverQuery, queryError)
        || !clearCoverStore.load(clearCoverQuery, queryError)) {
        context.initializationError = initializationFailure(QStringLiteral("loading the cover-cache queries"), queryError);
        return context;
    }
    context.coverCacheRepository = std::make_unique<SqliteCoverCacheRepository>(
        context.database->connection(), std::move(readCoverQuery), std::move(upsertCoverQuery),
        std::move(deleteCoverQuery), std::move(clearCoverQuery));
    const QString cacheRoot = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath("covers");
    const QString temporaryRoot = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath("HaikenAnime/covers");
    context.coverFileStore = std::make_unique<CoverFileStore>(cacheRoot, settings.covers);
    context.coverDownloader = std::make_unique<QtCoverDownloader>(temporaryRoot, settings.covers);
    context.coverCoordinator = std::make_unique<CoverDownloadCoordinator>(
        *context.coverDownloader, *context.coverCacheRepository, *context.coverFileStore, settings.covers);
    context.coverCoordinator->setLogger(context.logger.get());
    context.coverQuality = settings.covers.quality;
    context.initialSync = std::make_unique<InitialSyncCoordinator>(
        context.database->databasePath(),
        QStringLiteral(":/fixtures/graphql/page-response.json"),
        queryConfiguration.upsertMediaPath, queryConfiguration.readMediaPath,
        queryConfiguration.readActiveMediaIdsPath,
        queryConfiguration.markMediaSourceRemovedPath,
        settings.syncTimeoutMs, settings.syncIntervalMs);
    context.initialSync->setLogger(context.logger.get());
    context.logger->info(LogCategory::Application, QStringLiteral("Application composition completed."));
    return context;
}

bool scheduleStartupLibraryScan(ApplicationContext &context, QObject *lifetime) {
    if (!lifetime || !context.localLibraryScan || context.startupLibraryScanScheduled) return false;
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
