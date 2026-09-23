#ifndef HAIKENANIME_ANILISTPENDINGCHANGESTATUS_H
#define HAIKENANIME_ANILISTPENDINGCHANGESTATUS_H

/** Describes the lifecycle state of a local change awaiting AniList synchronization. */
enum class AniListPendingChangeStatus {
    Pending,
    Processing,
    Succeeded,
    Failed,
    RequiresConfirmation
};

#endif // HAIKENANIME_ANILISTPENDINGCHANGESTATUS_H
