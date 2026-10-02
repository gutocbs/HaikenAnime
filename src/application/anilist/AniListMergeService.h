#ifndef HAIKENANIME_ANILISTMERGESERVICE_H
#define HAIKENANIME_ANILISTMERGESERVICE_H

#include "AniListMergePolicyResolver.h"
#include "../../domain/anilist/AniListMergeDecision.h"

/** Applies the configured field policy to local and remote values. */
class AniListMergeService final {
public:
    /** Merges one field without persisting changes or contacting AniList. */
    [[nodiscard]] static AniListMergeDecision Merge(
        AniListField field, const AniListFieldValue &localValue,
        const AniListFieldValue &remoteValue);
};

#endif // HAIKENANIME_ANILISTMERGESERVICE_H
