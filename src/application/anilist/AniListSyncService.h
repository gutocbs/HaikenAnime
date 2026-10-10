#ifndef HAIKENANIME_ANILIST_SYNCSERVICE_H
#define HAIKENANIME_ANILIST_SYNCSERVICE_H

#include "../media/IMediaDataSource.h"
#include "../media/IMediaWriter.h"
#include "../media/IMediaSnapshotReconciler.h"
#include "../media/MediaPage.h"
#include "../media/MediaSyncFilter.h"
#include "IAniListDataSource.h"
#include "AniListSyncErrorCategory.h"

#include <functional>

class AniListPendingChangeProcessor;
class AniListPendingChangeReconciler;

class AniListSyncService {
public:
    using CheckpointCommitter = std::function<bool(int confirmedPage, QString &error)>;
    using CancellationProbe = std::function<bool()>;
    /** Creates a synchronization service from a data source and media repository. */
    AniListSyncService(IMediaDataSource &dataSource, IMediaWriter &mediaWriter,
                       IMediaSnapshotReconciler *snapshotReconciler = nullptr,
                       AniListPendingChangeProcessor *pendingProcessor = nullptr,
                       int timeoutMs = 0,
                       AniListPendingChangeReconciler *pendingReconciler = nullptr)
        : dataSource_(&dataSource), mediaWriter_(mediaWriter),
          snapshotReconciler_(snapshotReconciler), pendingProcessor_(pendingProcessor),
          pendingReconciler_(pendingReconciler), timeoutMs_(timeoutMs) {
    }

    AniListSyncService(IAniListDataSource &dataSource, IMediaWriter &mediaWriter,
                       IMediaSnapshotReconciler *snapshotReconciler = nullptr,
                       AniListPendingChangeProcessor *pendingProcessor = nullptr,
                       int timeoutMs = 0,
                       AniListPendingChangeReconciler *pendingReconciler = nullptr)
        : aniListDataSource_(&dataSource), mediaWriter_(mediaWriter),
          snapshotReconciler_(snapshotReconciler), pendingProcessor_(pendingProcessor),
          pendingReconciler_(pendingReconciler), timeoutMs_(timeoutMs) {
    }

    /** Fetches and persists all pages selected by filter, or all available data when empty. */
    [[nodiscard]] bool synchronize(const MediaSyncFilter &filter, QString &error,
                                   const CheckpointCommitter &commitCheckpoint = {},
                                   const CancellationProbe &isCancelled = {});
    [[nodiscard]] bool synchronize(const AniListDataSourceRequest &request, QString &error,
                                   const CheckpointCommitter &commitCheckpoint = {},
                                   const CancellationProbe &isCancelled = {});
    [[nodiscard]] AniListSyncErrorCategory lastErrorCategory() const { return lastErrorCategory_; }

private:
    IMediaDataSource *dataSource_ = nullptr;
    IAniListDataSource *aniListDataSource_ = nullptr;
    IMediaWriter &mediaWriter_;
    IMediaSnapshotReconciler *snapshotReconciler_;
    AniListPendingChangeProcessor *pendingProcessor_;
    AniListPendingChangeReconciler *pendingReconciler_;
    int timeoutMs_;
    AniListSyncErrorCategory lastErrorCategory_ = AniListSyncErrorCategory::None;
};

#endif // HAIKENANIME_ANILIST_SYNCSERVICE_H
