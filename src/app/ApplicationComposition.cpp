#include "ApplicationComposition.h"

#include "ApplicationSyncTaskExecutor.h"

#include "../infrastructure/database/SqlQueryStore.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "../infrastructure/database/SqliteMediaRepository.h"
#include "../infrastructure/database/SqlitePendingChangeRepository.h"
#include "../infrastructure/database/SqlitePersonalListChangeWriter.h"
#include "../infrastructure/database/SqliteQueries.h"
#include "../infrastructure/configuration/JsonSettingsReader.h"
#include "../infrastructure/database/SqliteCoverCacheRepository.h"
#include "../infrastructure/database/SqliteUserPreferencesRepository.h"
#include "../infrastructure/covers/CoverFileStore.h"
#include "../infrastructure/covers/CoverTemporaryStore.h"
#include "../infrastructure/covers/QtCoverDownloader.h"
#include "../infrastructure/library/LocalLibraryScanner.h"
#include "../infrastructure/library/QtLocalFileOpener.h"
#include "MpvAwareLocalFileOpener.h"
#include "../infrastructure/library/QtDirectoryEnumerator.h"
#include "../infrastructure/database/SqliteLocalFileRepository.h"
#include "../infrastructure/database/SqliteSyncTaskStateRepository.h"
#include "../infrastructure/anilist/AniListViewerClient.h"
#include "../infrastructure/anilist/WindowsAniListOAuthLauncher.h"
#include "../infrastructure/secrets/WindowsCredentialStore.h"
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

std::map<SyncTaskKind, SyncSchedulePolicy> backgroundSynchronizationPolicies(
    const std::map<SyncTaskKind, SyncSchedulePolicy> &configuredPolicies) {
    std::map<SyncTaskKind, SyncSchedulePolicy> policies;
    for (const auto kind : {SyncTaskKind::UserList, SyncTaskKind::PendingChange}) {
        const auto configured = configuredPolicies.find(kind);
        policies.emplace(kind, configured == configuredPolicies.end()
                                   ? DefaultSyncSchedulePolicy(kind) : configured->second);
    }
    return policies;
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
    explicit OwnedLocalFileRepository(std::unique_ptr<SqliteDatabase> database)
        : database_(std::move(database)), repository_(database_->connection(),
          SqliteQueries::BeginLibraryScan, SqliteQueries::UpsertLocalFile, SqliteQueries::CompleteLibraryScan,
          SqliteQueries::FailLibraryScan, SqliteQueries::MarkLocalFilesUnavailable,
          SqliteQueries::ReadPendingLocalFiles, SqliteQueries::ReadCatalogMediaForRecognition,
          SqliteQueries::SaveLocalFileRecognition, SqliteQueries::ReadLocalFilesUnderFolder,
          SqliteQueries::ReadLocalFileRoots) {}
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
    bool readFilesUnderFolder(const QString &folder, QList<LocalFileRecognitionRecord> &records,
                              QString &error) override {
        return repository_.readFilesUnderFolder(folder, records, error);
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
                                 QString upsertQuery)
        : database_(std::move(database)), repository_(database_->connection(), std::move(readQuery),
          std::move(upsertQuery)) {}
    bool ReadAll(QList<SyncTaskState> &states, QString &error) override { return repository_.ReadAll(states, error); }
    bool Upsert(const SyncTaskState &state, QString &error) override { return repository_.Upsert(state, error); }
private:
    std::unique_ptr<SqliteDatabase> database_;
    SqliteSyncTaskStateRepository repository_;
};

// Each scan and recognition worker opens its own SQLite connection on its thread.
LocalLibraryScanCoordinator::RepositoryFactory localFileRepositoryFactory(const QString &databasePath) {
    return [databasePath](QString &error) -> std::unique_ptr<ILocalFileRepository> {
        auto database = std::make_unique<SqliteDatabase>(databasePath);
        if (!database->open()) {
            error = database->lastError();
            return {};
        }
        return std::make_unique<OwnedLocalFileRepository>(std::move(database));
    };
}

void composeLibraryScanner(ApplicationContext &context) {
    context.localLibraryScan = std::make_unique<LocalLibraryScanCoordinator>(
        [](QString &) -> std::unique_ptr<ILocalLibraryScanner> {
            return std::make_unique<OwnedLibraryScanner>();
        },
        localFileRepositoryFactory(context.database->databasePath()));
    context.localLibraryScan->setLogger(context.logger.get());
}

void composeLibraryRecognition(ApplicationContext &context) {
    context.localLibraryRecognition = std::make_unique<LocalLibraryRecognitionCoordinator>(
        localFileRepositoryFactory(context.database->databasePath()));
    context.localLibraryRecognition->setLogger(context.logger.get());
    QObject::connect(context.localLibraryScan.get(), &LocalLibraryScanCoordinator::succeeded,
                     context.localLibraryRecognition.get(),
                     [recognition = context.localLibraryRecognition.get()](const QString &rootPath, qsizetype) {
                         recognition->start(rootPath);
                     });
}

void composeLocalEpisodeServices(ApplicationContext &context) {
    context.localEpisodeReader = std::make_unique<LocalEpisodeReader>(
        context.database->connection(), SqliteQueries::ReadNextLocalEpisode,
        SqliteQueries::ReadAvailableEpisodeCount);
    context.defaultLocalFileOpener = std::make_unique<QtLocalFileOpener>();
}

void composeEpisodeRecognition(ApplicationContext &context) {
    context.recognition = createRecognitionContext(
        {.connection = context.database->connection(),
         .mediaReader = context.mediaRepository.get(),
         .changes = context.personalListChangeService.get(),
         .diagnostics = [logger = context.logger.get()](const QString &message) {
             if (logger) logger->warning(LogCategory::Application, message);
         },
         .info = [logger = context.logger.get()](const QString &message) {
             if (logger) logger->info(LogCategory::Application, message);
         },
         .progressApplied = [listener = context.recognitionProgressListener](const Media &media) {
             if (*listener) (*listener)(media);
         }});
    for (const auto &warning : context.recognition->warnings) {
        context.logger->warning(LogCategory::Application, warning);
    }
    if (context.defaultLocalFileOpener) {
        context.localFileOpener = std::make_unique<MpvAwareLocalFileOpener>(*context.defaultLocalFileOpener,
                                                                            *context.recognition);
    }
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

    auto mediaRepository = std::make_unique<SqliteMediaRepository>(context.database->connection());
    mediaRepository->setLogger(context.logger.get());
    context.mediaRepository = std::move(mediaRepository);
    context.pendingChangeRepository = std::make_unique<SqlitePendingChangeRepository>(context.database->connection());
    auto personalListChangeWriter = std::make_unique<SqlitePersonalListChangeWriter>(context.database->connection());
    personalListChangeWriter->setLogger(context.logger.get());
    context.personalListChangeWriter = std::move(personalListChangeWriter);
    context.personalListChangeService = std::make_unique<PersonalListChangeService>(
        *context.personalListChangeWriter);

    composeLibraryScanner(context);
    composeLibraryRecognition(context);
    composeLocalEpisodeServices(context);
    composeEpisodeRecognition(context);

    {
        auto preferencesRepository = std::make_unique<SqliteUserPreferencesRepository>(context.database->connection());
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
        context.database->connection(), SqliteQueries::ReadCoverCache,
        SqliteQueries::UpsertCoverCache, SqliteQueries::DeleteCoverCache,
        SqliteQueries::ClearCoverCache);
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
    context.aniListSecretStore = std::make_unique<WindowsCredentialStore>();
    context.aniListAuthManager = std::make_unique<AniListAuthManager>(*context.aniListSecretStore);
    context.aniListAuthManager->setAuditLogger([logger = context.logger.get()](const QString &event) {
        if (logger) logger->info(LogCategory::Application, event);
    });
    QString credentialsError;
    static_cast<void>(context.aniListAuthManager->load(credentialsError));
    if (!settings.aniList.oauthClientId.trimmed().isEmpty()
        && settings.aniList.oauthRedirectUri.isValid()
        && !settings.aniList.oauthRedirectUri.scheme().isEmpty()) {
        context.aniListOAuthConfiguration.emplace(settings.aniList.oauthClientId,
                                                  settings.aniList.oauthRedirectUri);
        context.aniListNetworkManager = std::make_unique<QNetworkAccessManager>();
        const AniListGraphQlClient::Diagnostics oauthDiagnostics{
            .log = [logger = context.logger.get()](const QString &event) {
                if (logger) logger->info(LogCategory::Sync, event);
            }};
        context.aniListGraphQlClient = std::make_unique<AniListGraphQlClient>(
            *context.aniListNetworkManager, context.aniListAuthManager.get(),
            QUrl(settings.aniList.endpoint), settings.http.timeoutMs,
            settings.http.maxRetries, settings.http.retryDelayMs, oauthDiagnostics);
        context.aniListViewerQueryStore = std::make_unique<GraphQlQueryStore>(
            QStringLiteral(":/anilist/queries/viewer.graphql"));
        context.aniListViewerClient = std::make_unique<AniListViewerClient>(
            *context.aniListGraphQlClient, *context.aniListViewerQueryStore);
        context.aniListOAuthLauncher = std::make_unique<WindowsAniListOAuthLauncher>();
    }
    const auto synchronizationDatabasePath = context.database->databasePath();
    const auto readTaskStatesQueryPath = SqliteQueries::ReadSyncTaskStates;
    const auto upsertTaskStateQueryPath = SqliteQueries::UpsertSyncTaskState;
    auto *authProvider = context.aniListAuthManager.get();
    const auto userListConfiguration = ApplicationSyncTaskExecutor::AuthenticatedUserListConfiguration{
        QUrl(settings.aniList.endpoint), context.aniListAuthManager->credentials(),
        [authProvider] { return authProvider ? authProvider->credentials() : AniListCredentials{}; },
        settings.aniList.userListPerChunk, settings.http.timeoutMs, settings.http.maxRetries,
        settings.http.retryDelayMs,
        [logger = context.logger.get()](const QString &event) {
            if (logger) logger->info(LogCategory::Sync, event);
        }};
    context.adaptiveSync = std::make_unique<AdaptiveSyncRuntime>(
        [synchronizationDatabasePath, readTaskStatesQueryPath, upsertTaskStateQueryPath](QString &error) -> std::unique_ptr<ISyncTaskStateRepository> {
            auto database = std::make_unique<SqliteDatabase>(synchronizationDatabasePath);
            if (!database->open()) {
                error = database->lastError();
                return {};
            }
            return std::make_unique<OwnedSyncTaskStateRepository>(std::move(database), readTaskStatesQueryPath,
                                                                   upsertTaskStateQueryPath);
        },
        [synchronizationDatabasePath, readTaskStatesQueryPath, upsertTaskStateQueryPath,
         timeoutMs = settings.syncTimeoutMs, userListConfiguration] {
            return std::make_unique<ApplicationSyncTaskExecutor>(
                synchronizationDatabasePath, readTaskStatesQueryPath, upsertTaskStateQueryPath,
                timeoutMs, userListConfiguration);
        }, backgroundSynchronizationPolicies(settings.syncTaskPolicies));
    context.seasonalNetworkManager = std::make_unique<QNetworkAccessManager>();
    const AniListGraphQlClient::Diagnostics seasonalDiagnostics{
        .log = [logger = context.logger.get()](const QString &event) {
            if (logger) logger->info(LogCategory::Sync, event);
        }};
    context.seasonalGraphQlClient = std::make_unique<AniListGraphQlClient>(
        *context.seasonalNetworkManager, nullptr, QUrl(settings.aniList.endpoint), settings.http.timeoutMs,
        settings.http.maxRetries, settings.http.retryDelayMs, seasonalDiagnostics);
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
    const auto requests = BuildLocalLibraryScanRequests(context.userPreferences.libraryRoots,
                                                        context.userPreferences.scanExtensions);
    const QPointer<LocalLibraryScanCoordinator> coordinator(context.localLibraryScan.get());
    QTimer::singleShot(0, lifetime, [coordinator, requests] {
        if (coordinator) coordinator->startAll(requests);
    });
    return true;
}
