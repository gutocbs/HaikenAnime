#ifndef HAIKENANIME_CLEARLOCALCACHEUSECASE_H
#define HAIKENANIME_CLEARLOCALCACHEUSECASE_H

#include "CoverDownloadCoordinator.h"
#include "../application/covers/ICoverTemporaryStore.h"

#include <QList>
#include <QString>

#include <functional>

struct CacheCleanupFailure final {
    QString component;
    QString error;
};

struct ClearLocalCacheResult final {
    int removedCoverFiles = 0;
    int removedCoverTemporaryFiles = 0;
    int removedCoverEntries = 0;
    QList<CacheCleanupFailure> failures;

    [[nodiscard]] bool Succeeded() const { return failures.isEmpty(); }
    [[nodiscard]] int TotalRemoved() const
    {
        return removedCoverFiles + removedCoverTemporaryFiles + removedCoverEntries;
    }
};

/** Clears the cover cache (entries, files and abandoned downloads); primary data is never touched. */
class ClearLocalCacheUseCase final {
public:
    using Completion = std::function<void(ClearLocalCacheResult)>;

    ClearLocalCacheUseCase(CoverDownloadCoordinator &covers, const ICoverTemporaryStore &temporaryFiles);

    /** Runs synchronously and reports through completion; returns false while a cleanup is already running. */
    bool Start(Completion completion);

private:
    CoverDownloadCoordinator &covers_;
    const ICoverTemporaryStore &temporaryFiles_;
    bool running_ = false;
};

#endif // HAIKENANIME_CLEARLOCALCACHEUSECASE_H
