#ifndef HAIKENANIME_ANILISTMERGEDECISION_H
#define HAIKENANIME_ANILISTMERGEDECISION_H

#include "AniListMergePolicy.h"
#include "AniListMergeResult.h"
#include "AniListPendingChange.h"

/** Contains the selected value and outcome of merging one field. */
struct AniListMergeDecision {
    AniListFieldValue value = QString();
    AniListMergePolicy policy = AniListMergePolicy::RemoteWins;
    AniListMergeResult result = AniListMergeResult::Ignored;
};

#endif // HAIKENANIME_ANILISTMERGEDECISION_H
