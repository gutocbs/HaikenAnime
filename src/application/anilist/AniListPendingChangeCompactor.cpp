#include "AniListPendingChangeCompactor.h"

#include "AniListMergePolicyResolver.h"

#include <algorithm>

QList<AniListPendingChange> AniListPendingChangeCompactor::Compact(
    QList<AniListPendingChange> changes) {
    std::sort(changes.begin(), changes.end(), [](const auto &left, const auto &right) {
        if (left.localUpdatedAt != right.localUpdatedAt) {
            return left.localUpdatedAt < right.localUpdatedAt;
        }
        if (left.createdAt != right.createdAt) {
            return left.createdAt < right.createdAt;
        }
        return left.id < right.id;
    });

    QList<AniListPendingChange> compacted;
    for (const auto &change : changes) {
        if (AniListMergePolicyResolver::PolicyFor(change.field) !=
            AniListMergePolicy::LocalWinsLatest) {
            compacted.append(change);
            continue;
        }

        for (auto &existing : compacted) {
            if (existing.mediaId == change.mediaId && existing.field == change.field) {
                existing.status = AniListPendingChangeStatus::Superseded;
            }
        }
        compacted.append(change);
    }

    return compacted;
}
