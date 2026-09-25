#ifndef HAIKENANIME_ANILIST_SYNCSERVICE_H
#define HAIKENANIME_ANILIST_SYNCSERVICE_H

#include "../media/IMediaDataSource.h"
#include "../media/IMediaWriter.h"
#include "../media/IMediaSnapshotReconciler.h"
#include "../media/MediaPage.h"
#include "../media/MediaSyncFilter.h"
#include "AniListSyncErrorCategory.h"

class AniListPendingChangeProcessor;

class AniListSyncService {
public:
    /** Creates a synchronization service from a data source and media repository. */
    AniListSyncService(IMediaDataSource &dataSource, IMediaWriter &mediaWriter,
                       IMediaSnapshotReconciler *snapshotReconciler = nullptr,
                       AniListPendingChangeProcessor *pendingProcessor = nullptr,
                       int timeoutMs = 0)
        : dataSource_(dataSource), mediaWriter_(mediaWriter),
          snapshotReconciler_(snapshotReconciler), pendingProcessor_(pendingProcessor),
          timeoutMs_(timeoutMs) {
    }

    /** Fetches and persists all pages selected by filter, or all available data when empty. */
    [[nodiscard]] bool synchronize(const MediaSyncFilter &filter, QString &error);
    [[nodiscard]] AniListSyncErrorCategory lastErrorCategory() const { return lastErrorCategory_; }

private:
    IMediaDataSource &dataSource_;
    IMediaWriter &mediaWriter_;
    IMediaSnapshotReconciler *snapshotReconciler_;
    AniListPendingChangeProcessor *pendingProcessor_;
    int timeoutMs_;
    AniListSyncErrorCategory lastErrorCategory_ = AniListSyncErrorCategory::None;
};

#endif // HAIKENANIME_ANILIST_SYNCSERVICE_H
