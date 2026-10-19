#ifndef HAIKENANIME_RECOGNITIONSYNCFEEDBACK_H
#define HAIKENANIME_RECOGNITIONSYNCFEEDBACK_H

#include <QString>

#include <functional>
#include <optional>

#include "../anilist/IAniListPendingChangeObserver.h"
#include "RecognitionHistoryService.h"

/// Feeds the AniList outbox outcome of Progress and Deletion changes back into the recognition history, so the
/// history shows whether the local progress reached AniList. Never touches the outbox or local progress.
///
/// A Progress change of media M with value P settles every MarkedWatched/Corrected record awaiting sync
/// whose effective media is M and effective episode is <= P: the outbox compacts consecutive progress
/// edits into the latest one, so earlier episodes travel inside it. A Deletion change of media M settles
/// the manual Removed records of M awaiting sync.
class RecognitionSyncFeedback final : public IAniListPendingChangeObserver {
public:
    explicit RecognitionSyncFeedback(RecognitionHistoryService &history,
                                     std::function<void(const QString &)> logger = {});

    void changesSent(const QList<AniListPendingChange> &changes) override;
    void changesFailed(const QList<AniListPendingChange> &changes, AniListSyncErrorCategory category) override;

    /// Credential-free key of a synchronization failure ("retryable", "unauthorized", "rejected",
    /// "persistence", "unknown"), stored in the history and translated by the history screen.
    [[nodiscard]] static QString SafeError(AniListSyncErrorCategory category);

private:
    void settle(const QList<AniListPendingChange> &changes, const std::optional<QString> &safeError);
    void log(const QString &message) const;

    RecognitionHistoryService &history_;
    std::function<void(const QString &)> logger_;
};

#endif // HAIKENANIME_RECOGNITIONSYNCFEEDBACK_H
