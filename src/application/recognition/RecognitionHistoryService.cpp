#include "RecognitionHistoryService.h"

#include <utility>

namespace {
const auto ProgressWriteFailed = QStringLiteral("progress-write-failed");
// Records of one media still awaiting AniList are few (one per unsynced episode); the cap only bounds a query.
constexpr int AwaitingSyncLimit = 500;
}

RecognitionHistoryService::RecognitionHistoryService(IRecognitionHistoryRepository &repository,
                                                     IRecognitionProgressWriter &progress,
                                                     std::function<QDateTime()> clock)
    : repository_(repository), progress_(progress), clock_(std::move(clock)) {}

std::optional<RecognitionRecord> RecognitionHistoryService::record(const PlaybackSession &session,
                                                                   const EpisodeCandidate &candidate,
                                                                   const ProgressDecision &decision,
                                                                   QString &error) {
    error.clear();
    auto record = RecognitionRecord::create(session, candidate, now());
    QList<RecognitionRecord> existing;
    if (!repository_.find({.deduplicationKey = record.deduplicationKey(), .limit = 1}, existing, error)) {
        return std::nullopt;
    }
    // Pending means another call already reserved this session/episode; any outcome is final.
    if (!existing.isEmpty() && existing.first().status() != RecognitionStatus::Incomplete) return existing.first();

    // Reserve the deduplication key before touching progress so a retry can never apply it twice.
    if (existing.isEmpty()) {
        if (!repository_.insert(record, error)) return std::nullopt;
    } else {
        record.assignId(existing.first().id());
        if (!repository_.update(record, error)) return std::nullopt;
    }

    switch (decision.kind) {
    case ProgressDecisionKind::MarkWatched: {
        // The AniList drain may run as soon as the change is queued, so the record must already be visible
        // as awaiting sync; otherwise its markSynced would find nothing and the record stays pending forever.
        auto marked = record;
        marked.transitionTo(RecognitionStatus::MarkedWatched, now());
        marked.markSyncPending(now());
        if (!repository_.update(marked, error)) return std::nullopt;
        if (applyProgress(record, decision)) return marked;
        record.transitionTo(RecognitionStatus::Uncertain, now());
        break;
    }
    case ProgressDecisionKind::AlreadyWatched:
        record.transitionTo(RecognitionStatus::AlreadyWatched, now());
        break;
    case ProgressDecisionKind::Uncertain:
    case ProgressDecisionKind::Ignored:
        // Unknown media stays visible as uncertain so the user can pick the right anime.
        record.transitionTo(RecognitionStatus::Uncertain, now());
        record.setDiagnostic(decision.reason);
        break;
    }
    if (!repository_.update(record, error)) return std::nullopt;
    return record;
}

std::optional<RecognitionRecord> RecognitionHistoryService::recordIncomplete(const PlaybackSession &session,
                                                                             const EpisodeCandidate &candidate,
                                                                             QString &error) {
    error.clear();
    auto record = RecognitionRecord::create(session, candidate, now());
    QList<RecognitionRecord> existing;
    if (!repository_.find({.deduplicationKey = record.deduplicationKey(), .limit = 1}, existing, error)) {
        return std::nullopt;
    }
    if (!existing.isEmpty()) return existing.first();
    record.transitionTo(RecognitionStatus::Incomplete, now());
    if (!repository_.insert(record, error)) return std::nullopt;
    return record;
}

bool RecognitionHistoryService::list(const RecognitionHistoryFilter &filter, const RecognitionPage &page,
                                     QList<RecognitionRecord> &records, QString &error) {
    return repository_.find({.status = filter.status, .pluginId = filter.pluginId, .beforeId = page.beforeId,
                             .limit = page.limit},
                            records, error);
}

bool RecognitionHistoryService::correct(const qint64 recordId, const EpisodeCandidate &correction,
                                        const ProgressDecision &decision, QString &error) {
    auto record = load(recordId, error);
    if (!record.has_value()) return false;
    if (!record->correct(correction, now())) {
        error = QStringLiteral("Recognition record %1 cannot be corrected yet.").arg(recordId);
        return false;
    }
    if (decision.kind == ProgressDecisionKind::MarkWatched && applyProgress(*record, decision)) {
        record->markSyncPending(now());
    }
    return repository_.update(*record, error);
}

bool RecognitionHistoryService::ignore(const qint64 recordId, QString &error) {
    auto record = load(recordId, error);
    if (!record.has_value()) return false;
    if (!record->ignore(now())) {
        error = QStringLiteral("Recognition record %1 is already ignored.").arg(recordId);
        return false;
    }
    return repository_.update(*record, error);
}

bool RecognitionHistoryService::markSyncFailure(const qint64 recordId, const QString &safeError, QString &error) {
    auto record = load(recordId, error);
    if (!record.has_value()) return false;
    record->markSyncFailed(safeError, now());
    return repository_.update(*record, error);
}

bool RecognitionHistoryService::markSynced(const qint64 recordId, QString &error) {
    auto record = load(recordId, error);
    if (!record.has_value()) return false;
    record->markSynced(now());
    return repository_.update(*record, error);
}

bool RecognitionHistoryService::findAwaitingSync(const int mediaId, const int upToEpisode,
                                                 QList<RecognitionRecord> &records, QString &error) {
    records.clear();
    QList<RecognitionRecord> found;
    if (!repository_.find({.mediaId = mediaId, .maxEpisode = upToEpisode, .awaitingSync = true,
                           .limit = AwaitingSyncLimit},
                          found, error)) {
        return false;
    }
    for (auto &record : found) {
        if (record.status() == RecognitionStatus::MarkedWatched || record.status() == RecognitionStatus::Corrected) {
            records.append(std::move(record));
        }
    }
    return true;
}

QDateTime RecognitionHistoryService::now() const {
    return clock_ ? clock_() : QDateTime::currentDateTimeUtc();
}

std::optional<RecognitionRecord> RecognitionHistoryService::load(const qint64 recordId, QString &error) {
    error.clear();
    QList<RecognitionRecord> records;
    if (!repository_.find({.recordId = recordId, .limit = 1}, records, error)) return std::nullopt;
    if (records.isEmpty()) {
        error = QStringLiteral("Recognition record %1 does not exist.").arg(recordId);
        return std::nullopt;
    }
    return records.first();
}

bool RecognitionHistoryService::applyProgress(RecognitionRecord &record, const ProgressDecision &decision) {
    QString writeError;
    if (!decision.previous.has_value() || !decision.updated.has_value()
        || !progress_.apply(*decision.previous, *decision.updated, writeError)) {
        record.setDiagnostic(ProgressWriteFailed);
        return false;
    }
    return true;
}

bool RecognitionHistoryService::remove(const qint64 recordId, QString &error) {
    const auto record = load(recordId, error);
    if (!record.has_value()) return false;
    // Only a change this recognition queued and AniList has not confirmed yet is discarded.
    const bool appliedProgress = record->status() == RecognitionStatus::MarkedWatched
        || record->status() == RecognitionStatus::Corrected;
    const bool awaitingSync = record->syncState() == RecognitionSyncState::Pending
        || record->syncState() == RecognitionSyncState::Failed;
    const auto &candidate = record->effectiveCandidate();
    std::optional<QueuedProgress> queued;
    if (appliedProgress && awaitingSync && candidate.mediaId()) {
        queued = QueuedProgress{.mediaId = *candidate.mediaId(), .episode = candidate.episode()};
    }
    if (!repository_.remove(recordId, queued, error)) {
        if (error.isEmpty()) error = QStringLiteral("Recognition record %1 could not be removed.").arg(recordId);
        return false;
    }
    return true;
}

bool RecognitionHistoryService::recoverInterrupted(QString &error) {
    error.clear();
    QList<RecognitionRecord> candidates;
    if (!repository_.find({.status = RecognitionStatus::Pending, .limit = AwaitingSyncLimit}, candidates, error)) {
        return false;
    }
    QList<RecognitionRecord> awaiting;
    if (!repository_.find({.awaitingSync = true, .limit = AwaitingSyncLimit}, awaiting, error)) return false;
    for (auto &record : awaiting) {
        // Failed means the drain already saw the change; only a never-queued Pending sync is a lie.
        if (record.syncState() != RecognitionSyncState::Pending) continue;
        const auto &candidate = record.effectiveCandidate();
        if (!candidate.mediaId()) continue;
        bool queued = false;
        if (!repository_.hasQueuedProgress({.mediaId = *candidate.mediaId(), .episode = candidate.episode()}, queued,
                                           error)) {
            return false;
        }
        if (!queued) candidates.append(std::move(record));
    }
    for (auto &record : candidates) {
        if (record.markInterrupted(now()) && !repository_.update(record, error)) return false;
    }
    return true;
}
