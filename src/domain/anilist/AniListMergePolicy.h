#ifndef HAIKENANIME_ANILISTMERGEPOLICY_H
#define HAIKENANIME_ANILISTMERGEPOLICY_H

/** Describes how a local and remote value are reconciled. */
enum class AniListMergePolicy {
    RemoteWins,
    LocalWins,
    LocalWinsLatest,
    MaxValue,
    QueueLocalChange,
    RequiresConfirmation
};

#endif // HAIKENANIME_ANILISTMERGEPOLICY_H
