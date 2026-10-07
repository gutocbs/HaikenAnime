#ifndef HAIKENANIME_CLEARLOCALCACHEUSECASE_H
#define HAIKENANIME_CLEARLOCALCACHEUSECASE_H

#include "ICacheCleanupParticipant.h"
#include "../covers/CoverDownloadCoordinator.h"

#include <QList>
#include <QString>

#include <atomic>
#include <functional>

enum class ClearLocalCacheState { Idle, Running, CancellationRequested };

struct CacheCleanupFailure final {
    QString participant;
    QString error;
};

struct ClearLocalCacheResult final {
    int removedCoverFiles = 0;
    int removedCoverEntries = 0;
    int removedDerivedItems = 0;
    QList<CacheCleanupFailure> failures;
    bool cancelled = false;

    [[nodiscard]] bool Succeeded() const { return !cancelled && failures.isEmpty(); }
    [[nodiscard]] int TotalRemoved() const
    {
        return removedCoverFiles + removedCoverEntries + removedDerivedItems;
    }
};

class ClearLocalCacheUseCase final {
public:
    using Completion = std::function<void(ClearLocalCacheResult)>;

    ClearLocalCacheUseCase(CoverDownloadCoordinator &covers,
                           QList<ICacheCleanupParticipant *> participants);

    bool Start(Completion completion);
    void Cancel();
    [[nodiscard]] ClearLocalCacheState State() const;

private:
    CoverDownloadCoordinator &covers_;
    QList<ICacheCleanupParticipant *> participants_;
    std::atomic<ClearLocalCacheState> state_ = ClearLocalCacheState::Idle;
};

#endif // HAIKENANIME_CLEARLOCALCACHEUSECASE_H
