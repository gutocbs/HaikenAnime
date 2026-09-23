#ifndef HAIKENANIME_ANILISTPENDINGCHANGEGROUPER_H
#define HAIKENANIME_ANILISTPENDINGCHANGEGROUPER_H

#include <QList>

#include "AniListMediaPendingChanges.h"

/** Groups active pending changes into one update operation per media item. */
class AniListPendingChangeGrouper final {
public:
    /** Returns deterministic media groups ordered by media id and local update time. */
    [[nodiscard]] static QList<AniListMediaPendingChanges> Group(
        QList<AniListPendingChange> changes);
};

#endif // HAIKENANIME_ANILISTPENDINGCHANGEGROUPER_H
