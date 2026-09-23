#ifndef HAIKENANIME_ANILIST_SYNCSERVICE_H
#define HAIKENANIME_ANILIST_SYNCSERVICE_H

#include "IAniListDataSource.h"
#include "IMediaRepository.h"
#include "../media/IMediaDataSource.h"
#include "../../domain/media/MediaPage.h"
#include "../../domain/media/MediaSyncFilter.h"
#include "../../domain/anilist/AniListSyncErrorCategory.h"

class AniListPendingChangeProcessor;

class AniListSyncService {
public:
    /** Creates a synchronization service from a data source and media repository. */
    AniListSyncService(IAniListDataSource &dataSource, IMediaRepository &mediaRepository,
                       AniListPendingChangeProcessor *pendingProcessor = nullptr,
                       int timeoutMs = 0)
        : dataSource_(dataSource), mediaRepository_(mediaRepository),
          pendingProcessor_(pendingProcessor), timeoutMs_(timeoutMs) {
    }

    /** Fetches and persists all pages selected by filter, or all available data when empty. */
    [[nodiscard]] bool Synchronize(const MediaSyncFilter &filter, QString &error);
    [[nodiscard]] AniListSyncErrorCategory LastErrorCategory() const { return lastErrorCategory_; }

private:
    IMediaDataSource &dataSource_;
    IMediaRepository &mediaRepository_;
    AniListPendingChangeProcessor *pendingProcessor_;
    int timeoutMs_;
    AniListSyncErrorCategory lastErrorCategory_ = AniListSyncErrorCategory::None;
};

#endif // HAIKENANIME_ANILIST_SYNCSERVICE_H
