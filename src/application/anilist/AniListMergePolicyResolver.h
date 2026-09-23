#ifndef HAIKENANIME_ANILISTMERGEPOLICYRESOLVER_H
#define HAIKENANIME_ANILISTMERGEPOLICYRESOLVER_H

#include "../../domain/anilist/AniListField.h"
#include "../../domain/anilist/AniListMergePolicy.h"

/** Resolves the synchronization policy assigned to each AniList field. */
class AniListMergePolicyResolver final {
public:
    /** Returns the explicit merge policy for field. */
    [[nodiscard]] static AniListMergePolicy PolicyFor(AniListField field);
};

#endif // HAIKENANIME_ANILISTMERGEPOLICYRESOLVER_H
