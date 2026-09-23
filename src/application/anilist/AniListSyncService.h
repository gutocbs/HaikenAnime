#ifndef HAIKENANIME_ANILIST_SYNCSERVICE_H
#define HAIKENANIME_ANILIST_SYNCSERVICE_H

#include "IAniListDataSource.h"
#include "IMediaRepository.h"
#include "../media/IMediaDataSource.h"
#include "../../domain/media/MediaPage.h"
#include "../../domain/media/MediaSyncFilter.h"

class AniListSyncService {
public:
    /** Creates a synchronization service from a data source and media repository. */
    AniListSyncService(IAniListDataSource &dataSource, IMediaRepository &mediaRepository)
        : dataSource_(dataSource), mediaRepository_(mediaRepository) {
    }

    /** Fetches and persists all pages selected by filter, or all available data when empty. */
    [[nodiscard]] bool Synchronize(const MediaSyncFilter &filter, QString &error);

private:
    IMediaDataSource &dataSource_;
    IMediaRepository &mediaRepository_;
};

#endif // HAIKENANIME_ANILIST_SYNCSERVICE_H
