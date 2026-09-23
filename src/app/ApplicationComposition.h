#ifndef HAIKENANIME_APPLICATIONCOMPOSITION_H
#define HAIKENANIME_APPLICATIONCOMPOSITION_H

#include <memory>

#include "../application/anilist/IMediaRepository.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "InitialSyncCoordinator.h"
#include "../infrastructure/logging/AsyncLogger.h"


struct ApplicationContext final {
    std::unique_ptr<AsyncLogger> logger;
    std::unique_ptr<SqliteDatabase> database;
    std::unique_ptr<IMediaRepository> mediaRepository;
    std::unique_ptr<InitialSyncCoordinator> initialSync;
    QString initializationError;

    [[nodiscard]] bool isReady() const {
        return mediaRepository != nullptr && initializationError.isEmpty();
    }
};

[[nodiscard]] ApplicationContext createApplicationContext();

#endif // HAIKENANIME_APPLICATIONCOMPOSITION_H
