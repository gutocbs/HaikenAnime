#include "ApplicationComposition.h"

#include "ApplicationSyncTaskExecutor.h"

#include "../infrastructure/database/SqliteLocalEpisodeReader.h"
#include "../infrastructure/database/SqliteMediaRepository.h"
#include "../infrastructure/database/SqlitePendingChangeRepository.h"
#include "../infrastructure/database/SqlitePersonalListChangeWriter.h"
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

// Worker-owned factory products keep their borrowed dependency alive by inheriting it first: a base is
// built before and destroyed after the class that borrows it, so the scanner/repository always goes
// before its enumerator/connection.
struct OwnedEnumerator {
    QtDirectoryEnumerator enumerator;
};

class OwnedLibraryScanner final : private OwnedEnumerator, public LocalLibraryScanner {
public:
    OwnedLibraryScanner() : LocalLibraryScanner(enumerator) {}
};

struct OwnedConnection {
    explicit OwnedConnection(std::unique_ptr<SqliteDatabase> database) : database(std::move(database)) {}
    std::unique_ptr<SqliteDatabase> database;
};

template <typename Repository>
class ConnectionOwningRepository final : private OwnedConnection, public Repository {
public:
    explicit ConnectionOwningRepository(std::unique_ptr<SqliteDatabase> database)
        : OwnedConnection(std::move(database)), Repository(OwnedConnection::database->connection()) {}
};

// Each scan and recognition worker opens its own SQLite connection on its thread.
template <typename Repository>
std::function<std::unique_ptr<Repository>(QString &)> localFileRepositoryFactory(const QString &databasePath) {
    return [databasePath](QString &error) -> std::unique_ptr<Repository> {
        auto database = std::make_unique<SqliteDatabase>(databasePath);
        if (!database->open()) {
            error = database->lastError();
            return {};
        }
        return std::make_unique<ConnectionOwningRepository<SqliteLocalFileRepository>>(std::move(database));
    };
}

void composeLibraryScanner(ApplicationContext &context) {
    context.localLibraryScan = std::make_unique<LocalLibraryScanCoordinator>(
        [](QString &) -> std::unique_ptr<ILocalLibraryScanner> {
            return std::make_unique<OwnedLibraryScanner>();
        },
        localFileRepositoryFactory<ILocalFileScanRepository>(context.database->databasePath()));
    context.localLibraryScan->setLogger(context.logger.get());
}

void composeLibraryRecognition(ApplicationContext &context) {
    context.localLibraryRecognition = std::make_unique<LocalLibraryRecognitionCoordinator>(
        localFileRepositoryFactory<ILocalFileRecognitionRepository>(context.database->databasePath()));
    context.localLibraryRecognition->setLogger(context.logger.get());
    QObject::connect(context.localLibraryScan.get(), &LocalLibraryScanCoordinator::succeeded,
                     context.localLibraryRecognition.get(),
                     [recognition = context.localLibraryRecognition.get()](const QString &rootPath, qsizetype) {
                         recognition->startForPending(rootPath);
                     });
}

void composeLocalEpisodeServices(ApplicationContext &context) {
    context.localEpisodeReader = std::make_unique<SqliteLocalEpisodeReader>(context.database->connection());
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

    context.coverCacheRepository = std::make_unique<SqliteCoverCacheRepository>(context.database->connection());
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
        *context.coverCoordinator, *context.coverTemporaryStore);
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
    auto *authProvider = context.aniListAuthManager.get();
    const auto userListConfiguration = ApplicationSyncTaskExecutor::AuthenticatedUserListConfiguration{
        QUrl(settings.aniList.endpoint),
        [authProvider] { return authProvider ? authProvider->credentials() : AniListCredentials{}; },
        settings.aniList.userListPerChunk, settings.http.timeoutMs, settings.http.maxRetries,
        settings.http.retryDelayMs,
        [logger = context.logger.get()](const QString &event) {
            if (logger) logger->info(LogCategory::Sync, event);
        }};
    context.adaptiveSync = std::make_unique<AdaptiveSyncRuntime>(
        [synchronizationDatabasePath](QString &error) -> std::unique_ptr<ISyncTaskStateRepository> {
            auto database = std::make_unique<SqliteDatabase>(synchronizationDatabasePath);
            if (!database->open()) {
                error = database->lastError();
                return {};
            }
            return std::make_unique<ConnectionOwningRepository<SqliteSyncTaskStateRepository>>(std::move(database));
        },
        [synchronizationDatabasePath, timeoutMs = settings.syncTimeoutMs, userListConfiguration] {
            return std::make_unique<ApplicationSyncTaskExecutor>(
                synchronizationDatabasePath, timeoutMs, userListConfiguration);
        }, settings.syncTaskPolicies);
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
