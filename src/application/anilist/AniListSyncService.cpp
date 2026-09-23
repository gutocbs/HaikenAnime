#include "AniListSyncService.h"
#include "../../infrastructure/anilist/AniListMediaMapper.h"

bool AniListSyncService::Synchronize(const AniListSyncFilter &filter, QString &error) {
    AniListSyncFilter pageFilter = filter;
    pageFilter.startingPage = qMax(1, pageFilter.startingPage);
    pageFilter.perPage = qMax(1, pageFilter.perPage);

    while (true) {
        AniListPage page;
        if (!dataSource_.fetchPage(pageFilter, page, error)) {
            return false;
        }

        QList<Media> media;
        media.reserve(page.externalMedia.size());
        for (const auto &externalMedia : page.externalMedia) {
            media.append(AniListMediaMapper::ToDomainMedia(externalMedia));
        }

        if (!media.isEmpty() && !mediaRepository_.Upsert(media, error)) {
            return false;
        }

        if (!page.hasNextPage) {
            return true;
        }

        pageFilter.startingPage = page.currentPage + 1;
    }
}
