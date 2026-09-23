#ifndef HAIKENANIME_ANILIST_SYNCSERVICE_H
#define HAIKENANIME_ANILIST_SYNCSERVICE_H

#include "IAniListDataSource.h"
#include "IMediaRepository.h"

class AniListSyncService {
public:
    /** Creates a synchronization service from a data source and media repository. */
    AniListSyncService(IAniListDataSource &dataSource, IMediaRepository &mediaRepository)
        : dataSource_(dataSource), mediaRepository_(mediaRepository) {
    }

    /** Fetches and persists all pages selected by filter, or all available data when empty. */
    [[nodiscard]] bool Synchronize(const AniListSyncFilter &filter, QString &error);

private:
    IAniListDataSource &dataSource_;
    IMediaRepository &mediaRepository_;
};

#endif // HAIKENANIME_ANILIST_SYNCSERVICE_H
