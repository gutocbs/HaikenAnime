#include "AniListSyncService.h"
bool AniListSyncService::Synchronize(const MediaSyncFilter &filter, QString &error) {
    MediaSyncFilter pageFilter = filter;
    pageFilter.startingPage = qMax(1, pageFilter.startingPage);
    pageFilter.perPage = qMax(1, pageFilter.perPage);

    while (true) {
        MediaPage page;
        if (!dataSource_.fetchPage(pageFilter, page, error)) {
            return false;
        }

        if (!page.media.isEmpty() && !mediaRepository_.Upsert(page.media, error)) {
            return false;
        }

        if (!page.hasNextPage) {
            return true;
        }

        pageFilter.startingPage = page.currentPage + 1;
    }
}
