#include "AniListPendingChangeGrouper.h"

#include <QMap>

#include <algorithm>

QList<AniListMediaPendingChanges> AniListPendingChangeGrouper::Group(
    QList<AniListPendingChange> changes) {
    std::sort(changes.begin(), changes.end(), [](const auto &left, const auto &right) {
        if (left.mediaId != right.mediaId) {
            return left.mediaId < right.mediaId;
        }
        if (left.localUpdatedAt != right.localUpdatedAt) {
            return left.localUpdatedAt < right.localUpdatedAt;
        }
        if (left.createdAt != right.createdAt) {
            return left.createdAt < right.createdAt;
        }
        return left.id < right.id;
    });

    QMap<int, AniListMediaPendingChanges> grouped;
    for (const auto &change : changes) {
        if (change.status == AniListPendingChangeStatus::Superseded) {
            continue;
        }
        auto &mediaChanges = grouped[change.mediaId];
        mediaChanges.mediaId = change.mediaId;
        mediaChanges.changes.append(change);
    }

    QList<AniListMediaPendingChanges> result;
    for (auto it = grouped.cbegin(); it != grouped.cend(); ++it) {
        result.append(it.value());
    }
    return result;
}
