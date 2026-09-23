#ifndef HAIKENANIME_ANILISTPENDINGCHANGECOMPACTOR_H
#define HAIKENANIME_ANILISTPENDINGCHANGECOMPACTOR_H

#include <QList>

#include "../../domain/anilist/AniListPendingChange.h"

/** Removes obsolete local changes while preserving the newest change per field. */
class AniListPendingChangeCompactor final {
public:
    /** Orders changes and marks replaced changes as superseded. */
    [[nodiscard]] static QList<AniListPendingChange> Compact(
        QList<AniListPendingChange> changes);
};

#endif // HAIKENANIME_ANILISTPENDINGCHANGECOMPACTOR_H
