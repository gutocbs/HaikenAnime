#ifndef HAIKENANIME_ANILIST_SYNCSERVICE_H
#define HAIKENANIME_ANILIST_SYNCSERVICE_H

#include "IAniListDataSource.h"
#include "IMediaRepository.h"

class AniListSyncService {
public:
    AniListSyncService(IAniListDataSource &dataSource, IMediaRepository &mediaRepository)
        : dataSource_(dataSource), mediaRepository_(mediaRepository) {
    }

private:
    IAniListDataSource &dataSource_;
    IMediaRepository &mediaRepository_;
};

#endif // HAIKENANIME_ANILIST_SYNCSERVICE_H
