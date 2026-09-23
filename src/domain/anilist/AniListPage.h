#ifndef HAIKENANIME_ANILISTPAGE_H
#define HAIKENANIME_ANILISTPAGE_H

#include <QList>

#include "AniListMediaDto.h"
#include "../media/Media.h"

/** Represents one page of media and its pagination metadata. */
struct AniListPage {
    QList<Media> media;
    QList<AniListMediaDto> externalMedia;
    bool hasNextPage = false;
    int currentPage = 0;
    int totalPages = 0;
};

#endif // HAIKENANIME_ANILISTPAGE_H
