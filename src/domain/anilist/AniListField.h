#ifndef HAIKENANIME_ANILISTFIELD_H
#define HAIKENANIME_ANILISTFIELD_H

/**
 * Field targeted by a pending AniList change. The numeric values are stored in anilist_pending_changes
 * and must never change.
 */
enum class AniListField {
    Progress = 5,
    PersonalScore = 6,
    ListStatus = 7,
    Deletion = 9
};

#endif // HAIKENANIME_ANILISTFIELD_H
