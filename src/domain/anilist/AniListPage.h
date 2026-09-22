#ifndef HAIKENANIME_ANILISTPAGE_H
#define HAIKENANIME_ANILISTPAGE_H

#include <QList>

#include "../media/Media.h"

struct AniListPage {
    QList<Media> media;
    bool hasNextPage = false;
    int currentPage = 0;
    int totalPages = 0;
};

#endif // HAIKENANIME_ANILISTPAGE_H
