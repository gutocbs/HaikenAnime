#ifndef HAIKENANIME_ANILISTMEDIAPENDINGCHANGES_H
#define HAIKENANIME_ANILISTMEDIAPENDINGCHANGES_H

#include <QList>

#include "../../domain/anilist/AniListPendingChange.h"

/** Represents all active local changes that will be sent for one media item. */
struct AniListMediaPendingChanges {
    int mediaId = 0;
    QList<AniListPendingChange> changes;
};

#endif // HAIKENANIME_ANILISTMEDIAPENDINGCHANGES_H
