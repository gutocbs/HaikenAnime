#ifndef HAIKENANIME_ANILISTMERGERESULT_H
#define HAIKENANIME_ANILISTMERGERESULT_H

/** Describes the outcome of reconciling one synchronized field. */
enum class AniListMergeResult {
    Applied,
    Queued,
    Conflict,
    RequiresConfirmation,
    Ignored
};

#endif // HAIKENANIME_ANILISTMERGERESULT_H
