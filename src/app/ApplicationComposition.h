#ifndef HAIKENANIME_APPLICATIONCOMPOSITION_H
#define HAIKENANIME_APPLICATIONCOMPOSITION_H

#include <memory>
#include <optional>

#include <QNetworkAccessManager>

#include "../application/media/IMediaRepository.h"
#include "../application/anilist/IPendingChangeRepository.h"
#include "../application/media/IPersonalListChangeWriter.h"
#include "../application/media/PersonalListChangeService.h"
#include "../application/library/ILocalFileOpener.h"
#include "../application/library/LocalEpisodeReader.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "AdaptiveSyncRuntime.h"
#include "LocalLibraryScanCoordinator.h"
#include "LocalLibraryRecognitionCoordinator.h"
#include "../infrastructure/logging/AsyncLogger.h"
#include "../application/covers/CoverDownloadCoordinator.h"
#include "../application/covers/ICoverTemporaryStore.h"
#include "../application/cache/ClearLocalCacheUseCase.h"
#include "../application/configuration/IUserPreferencesRepository.h"
#include "../application/configuration/UserPreferences.h"
#include "../infrastructure/anilist/AniListGraphQlClient.h"
#include "../infrastructure/anilist/GraphQlQueryStore.h"
#include "../infrastructure/anilist/GraphQlSeasonalCatalogDataSource.h"
#include "../infrastructure/anilist/AniListViewerClient.h"
#include "../infrastructure/anilist/WindowsAniListOAuthLauncher.h"
#include "../application/anilist/AniListAuthManager.h"
#include "../application/anilist/ISecretStore.h"
#include "SeasonalCatalogCoordinator.h"
#include "RecognitionComposition.h"


struct ApplicationContext final {
    std::unique_ptr<AsyncLogger> logger;
    std::unique_ptr<SqliteDatabase> database;
    std::unique_ptr<IMediaRepository> mediaRepository;
    std::unique_ptr<IPendingChangeRepository> pendingChangeRepository;
    std::unique_ptr<IPersonalListChangeWriter> personalListChangeWriter;
    std::unique_ptr<PersonalListChangeService> personalListChangeService;
    std::unique_ptr<ISecretStore> aniListSecretStore;
    std::unique_ptr<AniListAuthManager> aniListAuthManager;
    std::optional<AniListOAuthConfig> aniListOAuthConfiguration;
    std::unique_ptr<QNetworkAccessManager> aniListNetworkManager;
    std::unique_ptr<AniListGraphQlClient> aniListGraphQlClient;
    std::unique_ptr<GraphQlQueryStore> aniListViewerQueryStore;
    std::unique_ptr<AniListViewerClient> aniListViewerClient;
    std::unique_ptr<WindowsAniListOAuthLauncher> aniListOAuthLauncher;
    std::unique_ptr<AdaptiveSyncRuntime> adaptiveSync;
    std::unique_ptr<ICoverCacheRepository> coverCacheRepository;
    std::unique_ptr<ICoverFileStore> coverFileStore;
    std::unique_ptr<ICoverTemporaryStore> coverTemporaryStore;
    std::unique_ptr<ICoverDownloader> coverDownloader;
    std::unique_ptr<CoverDownloadCoordinator> coverCoordinator;
    std::unique_ptr<ClearLocalCacheUseCase> clearLocalCache;
    std::unique_ptr<IUserPreferencesRepository> userPreferencesRepository;
    UserPreferences userPreferences;
    std::unique_ptr<QNetworkAccessManager> seasonalNetworkManager;
    std::unique_ptr<AniListGraphQlClient> seasonalGraphQlClient;
    std::unique_ptr<GraphQlQueryStore> seasonalQueryStore;
    std::unique_ptr<GraphQlSeasonalCatalogDataSource> seasonalCatalogDataSource;
    std::unique_ptr<SeasonalCatalogCoordinator> seasonalCatalogCoordinator;
    // Recognition depends on the repositories above and outlives the opener that launches mpv.
    std::unique_ptr<RecognitionContext> recognition;
    /// Set by the UI after composition; shared so it survives moving the context.
    std::shared_ptr<std::function<void(const Media &)>> recognitionProgressListener =
        std::make_shared<std::function<void(const Media &)>>();
    std::unique_ptr<ILocalEpisodeReader> localEpisodeReader;
    std::unique_ptr<ILocalFileOpener> defaultLocalFileOpener;
    std::unique_ptr<ILocalFileOpener> localFileOpener;
    QString initializationError;
    bool startupLibraryScanScheduled = false;
    // Declared last so destruction joins worker threads before other dependencies.
    std::unique_ptr<LocalLibraryRecognitionCoordinator> localLibraryRecognition;
    std::unique_ptr<LocalLibraryScanCoordinator> localLibraryScan;

    [[nodiscard]] QString schedulingInitializationError() const {
        return adaptiveSync ? adaptiveSync->initializationError() : QString{};
    }
    [[nodiscard]] bool isReady() const {
        return mediaRepository != nullptr && initializationError.isEmpty()
            && schedulingInitializationError().isEmpty();
    }
};

[[nodiscard]] ApplicationContext createApplicationContext();

struct ApplicationCompositionOptions final {
    QString databasePath;
    QString settingsPath = QStringLiteral(":/config/Settings.json");
};
[[nodiscard]] ApplicationContext createApplicationContext(const ApplicationCompositionOptions &options);
// Call only after the settings controller and QML have been initialized.
bool scheduleStartupLibraryScan(ApplicationContext &context, QObject *lifetime);

#endif // HAIKENANIME_APPLICATIONCOMPOSITION_H
