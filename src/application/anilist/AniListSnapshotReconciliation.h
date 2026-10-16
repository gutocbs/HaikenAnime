#ifndef HAIKENANIME_ANILISTSNAPSHOTRECONCILIATION_H
#define HAIKENANIME_ANILISTSNAPSHOTRECONCILIATION_H

#include "AniListSyncService.h"
#include "IPendingChangeRepository.h"
#include "../media/IMediaSnapshotReconciler.h"

#include <QList>
#include <QSet>
#include <QString>

/**
 * Marks media absent from a full ANIME + MANGA synchronization as removed from AniList.
 *
 * Reconciles only when the whole run succeeded (not cancelled) and every per-type outcome is a
 * complete snapshot (one per expected media type); media with unsent local changes are never marked. reconciled tells whether
 * the reconciler was invoked. Returns false only when reading exclusions or reconciling failed.
 */
[[nodiscard]] inline bool ReconcileCompleteAniListSnapshot(
    const QList<AniListSyncOutcome> &outcomes, const qsizetype expectedOutcomeCount,
    const bool runSucceededWithoutCancellation,
    IMediaSnapshotReconciler &snapshotReconciler, IPendingChangeRepository &pendingChanges,
    bool &reconciled, int &removedCount, QString &error) {
    reconciled = false;
    removedCount = 0;
    error.clear();
    if (!runSucceededWithoutCancellation || outcomes.size() != expectedOutcomeCount) return true;
    QSet<int> observedMediaIds;
    for (const auto &outcome : outcomes) {
        if (!outcome.completeSnapshot) return true;
        observedMediaIds.unite(outcome.observedMediaIds);
    }
    QSet<int> excludedMediaIds;
    if (!pendingChanges.getMediaIdsWithUnsentChanges(excludedMediaIds, error)) return false;
    if (!snapshotReconciler.reconcileAuthoritativeSnapshot(
            observedMediaIds, excludedMediaIds, removedCount, error)) {
        removedCount = 0;
        return false;
    }
    reconciled = true;
    return true;
}

#endif
