#ifndef HAIKENANIME_ANILISTSYNCERRORCATEGORY_H
#define HAIKENANIME_ANILISTSYNCERRORCATEGORY_H

/** Categorizes an application synchronization failure and whether a later run may retry it. */
enum class AniListSyncErrorCategory {
    None, Authentication, Authorization, RateLimit, GraphQl, Network, Timeout,
    Persistence, InvalidData, Cancelled, Unknown
};

[[nodiscard]] constexpr bool IsRetryable(AniListSyncErrorCategory category) {
    return category == AniListSyncErrorCategory::RateLimit
        || category == AniListSyncErrorCategory::Network
        || category == AniListSyncErrorCategory::Timeout;
}

#endif // HAIKENANIME_ANILISTSYNCERRORCATEGORY_H
