#ifndef HAIKENANIME_APPLICATIONCOMPOSITION_H
#define HAIKENANIME_APPLICATIONCOMPOSITION_H

#include <memory>

#include "../application/media/IMediaRepository.h"
#include "../application/anilist/IPendingChangeRepository.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "InitialSyncCoordinator.h"
#include "../infrastructure/logging/AsyncLogger.h"
#include "../application/covers/CoverDownloadCoordinator.h"
#include "../application/configuration/IUserPreferencesRepository.h"
#include "../application/configuration/UserPreferences.h"


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
    QString initializationError;

    [[nodiscard]] bool isReady() const {
        return mediaRepository != nullptr && initializationError.isEmpty();
    }
};

[[nodiscard]] ApplicationContext createApplicationContext();

#endif // HAIKENANIME_APPLICATIONCOMPOSITION_H
