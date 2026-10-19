#ifndef HAIKENANIME_ANILIST_SYNCSERVICE_H
#define HAIKENANIME_ANILIST_SYNCSERVICE_H

#include "../media/IMediaWriter.h"
#include "../media/MediaPage.h"
#include "../media/MediaSyncFilter.h"
#include "IAniListDataSource.h"
#include "AniListSyncErrorCategory.h"

#include <QSet>

#include <functional>

class AniListPendingChangeReconciler;

struct AniListSyncOutcome final {
    QSet<int> observedMediaIds;
    bool completeSnapshot = false;
};

class AniListSyncService {
public:
    using CheckpointCommitter = std::function<bool(int confirmedPage, QString &error)>;
    using CancellationProbe = std::function<bool()>;
    using ProgressReporter = std::function<void(int processedItems)>;
    /**
     * Creates a synchronization service from a data source and media repository. pendingReconciler, when
     * given, reconciles local pending changes with each page before it is persisted.
     */
    AniListSyncService(IAniListDataSource &dataSource, IMediaWriter &mediaWriter, int timeoutMs = 0,
                       AniListPendingChangeReconciler *pendingReconciler = nullptr)
        : dataSource_(dataSource), mediaWriter_(mediaWriter), pendingReconciler_(pendingReconciler),
          timeoutMs_(timeoutMs) {
    }

    /** Fetches and persists every page of the request, starting at its page. */
    [[nodiscard]] bool synchronize(const AniListDataSourceRequest &request, QString &error,
                                   const CheckpointCommitter &commitCheckpoint = {},
                                   const CancellationProbe &isCancelled = {});
    /** Reports the cumulative media count of the run after each persisted page. */
    void setProgressReporter(ProgressReporter reporter) { progressReporter_ = std::move(reporter); }
    [[nodiscard]] AniListSyncErrorCategory lastErrorCategory() const { return lastErrorCategory_; }
    /** Reports the entries observed by the last run and whether they form a complete snapshot. */
    [[nodiscard]] const AniListSyncOutcome &lastOutcome() const { return lastOutcome_; }

private:
    IAniListDataSource &dataSource_;
    IMediaWriter &mediaWriter_;
    AniListPendingChangeReconciler *pendingReconciler_;
    int timeoutMs_;
    AniListSyncErrorCategory lastErrorCategory_ = AniListSyncErrorCategory::None;
    AniListSyncOutcome lastOutcome_;
    ProgressReporter progressReporter_;
};

#endif // HAIKENANIME_ANILIST_SYNCSERVICE_H
