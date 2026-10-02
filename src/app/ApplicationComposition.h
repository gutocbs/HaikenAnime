#ifndef HAIKENANIME_APPLICATIONCOMPOSITION_H
#define HAIKENANIME_APPLICATIONCOMPOSITION_H

#include <memory>

#include <QNetworkAccessManager>

#include "../application/media/IMediaRepository.h"
#include "../application/anilist/IPendingChangeRepository.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "InitialSyncCoordinator.h"
#include "LocalLibraryScanCoordinator.h"
#include "../infrastructure/logging/AsyncLogger.h"
#include "../application/covers/CoverDownloadCoordinator.h"
#include "../application/configuration/IUserPreferencesRepository.h"
#include "../application/configuration/UserPreferences.h"
#include "../infrastructure/anilist/AniListGraphQlClient.h"
#include "../infrastructure/anilist/GraphQlQueryStore.h"
#include "../infrastructure/anilist/GraphQlSeasonalCatalogDataSource.h"
#include "SeasonalCatalogCoordinator.h"


struct ApplicationContext final {
    std::unique_ptr<AsyncLogger> logger;
    std::unique_ptr<SqliteDatabase> database;
    std::unique_ptr<IMediaRepository> mediaRepository;
    std::unique_ptr<IPendingChangeRepository> pendingChangeRepository;
    std::unique_ptr<InitialSyncCoordinator> initialSync;
    std::unique_ptr<ICoverCacheRepository> coverCacheRepository;
    std::unique_ptr<ICoverFileStore> coverFileStore;
    std::unique_ptr<ICoverDownloader> coverDownloader;
    std::unique_ptr<CoverDownloadCoordinator> coverCoordinator;
    std::unique_ptr<IUserPreferencesRepository> userPreferencesRepository;
    UserPreferences userPreferences;
    CoverQuality coverQuality = CoverQuality::Medium;
    std::unique_ptr<QNetworkAccessManager> seasonalNetworkManager;
    std::unique_ptr<AniListGraphQlClient> seasonalGraphQlClient;
    std::unique_ptr<GraphQlQueryStore> seasonalQueryStore;
    std::unique_ptr<GraphQlSeasonalCatalogDataSource> seasonalCatalogDataSource;
    std::unique_ptr<SeasonalCatalogCoordinator> seasonalCatalogCoordinator;
    QString initializationError;
    bool startupLibraryScanScheduled = false;
    // Declared last so destruction joins the worker before other dependencies.
    std::unique_ptr<LocalLibraryScanCoordinator> localLibraryScan;

    [[nodiscard]] bool isReady() const {
        return mediaRepository != nullptr && initializationError.isEmpty();
    }
};

[[nodiscard]] ApplicationContext createApplicationContext();

struct ApplicationCompositionOptions final {
    QString databasePath;
    QString settingsPath = QStringLiteral(":/config/Settings.json");
    QString queryConfigurationPath = QStringLiteral(":/sqlite/queries/sqlite-queries.json");
};
[[nodiscard]] ApplicationContext createApplicationContext(const ApplicationCompositionOptions &options);
// Call only after the settings controller and QML have been initialized.
bool scheduleStartupLibraryScan(ApplicationContext &context, QObject *lifetime);

#endif // HAIKENANIME_APPLICATIONCOMPOSITION_H
