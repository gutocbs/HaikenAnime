#ifndef HAIKENANIME_ANILISTFIELD_H
#define HAIKENANIME_ANILISTFIELD_H

/** Identifies a field that can participate in AniList synchronization. */
enum class AniListField {
    ExternalId,
    Title,
    AlternativeTitles,
    CoverUrl,
    Synopsis,
    Progress,
    PersonalScore,
    ListStatus,
    LocalFiles,
    Deletion
};

#endif // HAIKENANIME_ANILISTFIELD_H
