#ifndef HAIKENANIME_MEDIAPAGE_H
#define HAIKENANIME_MEDIAPAGE_H

#include <QList>

#include "Media.h"

struct MediaPage {
    QList<Media> media;
    bool hasNextPage = false;
    int currentPage = 0;
    int totalPages = 0;
};

#endif
