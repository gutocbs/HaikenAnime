#include "RecognitionRecord.h"

#include <utility>

bool CanTransition(const RecognitionStatus from, const RecognitionStatus to) {
    switch (to) {
    case RecognitionStatus::Pending:
        return false;
    case RecognitionStatus::Incomplete:
    case RecognitionStatus::MarkedWatched:
    case RecognitionStatus::AlreadyWatched:
    case RecognitionStatus::Uncertain:
        return from == RecognitionStatus::Pending;
    case RecognitionStatus::Ignored:
        return from != RecognitionStatus::Ignored;
    case RecognitionStatus::Corrected:
        // A still-pending observation has nothing to correct yet.
        return from != RecognitionStatus::Pending;
    }
    return false;
}

RecognitionRecord::RecognitionRecord(const PlaybackSession &session, const EpisodeCandidate &candidate,
                                     const QDateTime &createdAt)
    : m_sessionId(session.id), m_userId(session.userId), m_deviceId(session.deviceId), m_candidate(candidate),
      m_createdAt(createdAt), m_updatedAt(createdAt) {}

RecognitionRecord RecognitionRecord::create(const PlaybackSession &session, const EpisodeCandidate &candidate,
                                            const QDateTime &createdAt) {
    return RecognitionRecord(session, candidate, createdAt);
}

const EpisodeCandidate &RecognitionRecord::effectiveCandidate() const {
    return m_correction.has_value() ? *m_correction : m_candidate;
}

QString RecognitionRecord::deduplicationKey() const {
    return m_sessionId.key() + QLatin1Char('|') + m_candidate.episodeKey();
}

bool RecognitionRecord::transitionTo(const RecognitionStatus status, const QDateTime &at) {
    if (!CanTransition(m_status, status)) return false;
    m_status = status;
    m_updatedAt = at;
    return true;
}

bool RecognitionRecord::correct(const EpisodeCandidate &correction, const QDateTime &at) {
    if (!transitionTo(RecognitionStatus::Corrected, at)) return false;
    m_correction = correction;
    return true;
}

bool RecognitionRecord::ignore(const QDateTime &at) {
    return transitionTo(RecognitionStatus::Ignored, at);
}

bool RecognitionRecord::markInterrupted(const QDateTime &at) {
    const bool claimedSync = (m_status == RecognitionStatus::MarkedWatched || m_status == RecognitionStatus::Corrected)
        && m_syncState == RecognitionSyncState::Pending;
    if (m_status != RecognitionStatus::Pending && !claimedSync) return false;
    m_status = RecognitionStatus::Uncertain;
    m_syncState = RecognitionSyncState::NotRequired;
    m_syncError.clear();
    m_diagnostic = QStringLiteral("interrupted");
    m_updatedAt = at;
    return true;
}

namespace {
constexpr std::pair<RecognitionStatus, const char *> StatusNames[] = {
    {RecognitionStatus::Pending, "pending"},
    {RecognitionStatus::Incomplete, "incomplete"},
    {RecognitionStatus::MarkedWatched, "marked-watched"},
    {RecognitionStatus::AlreadyWatched, "already-watched"},
    {RecognitionStatus::Uncertain, "uncertain"},
    {RecognitionStatus::Ignored, "ignored"},
    {RecognitionStatus::Corrected, "corrected"},
};

constexpr std::pair<RecognitionSyncState, const char *> SyncStateNames[] = {
    {RecognitionSyncState::NotRequired, "not-required"},
    {RecognitionSyncState::Pending, "pending"},
    {RecognitionSyncState::Failed, "failed"},
    {RecognitionSyncState::Synced, "synced"},
};

template <typename Enum, std::size_t Size>
QString NameOf(const std::pair<Enum, const char *> (&names)[Size], const Enum value) {
    for (const auto &[candidate, name] : names) {
        if (candidate == value) return QString::fromLatin1(name);
    }
    return {};
}

template <typename Enum, std::size_t Size>
std::optional<Enum> ValueOf(const std::pair<Enum, const char *> (&names)[Size], const QString &text) {
    for (const auto &[candidate, name] : names) {
        if (text == QLatin1String(name)) return candidate;
    }
    return std::nullopt;
}
}

QString ToString(const RecognitionStatus status) {
    return NameOf(StatusNames, status);
}

QString ToString(const RecognitionSyncState state) {
    return NameOf(SyncStateNames, state);
}

std::optional<RecognitionStatus> RecognitionStatusFromString(const QString &value) {
    return ValueOf(StatusNames, value);
}

std::optional<RecognitionSyncState> RecognitionSyncStateFromString(const QString &value) {
    return ValueOf(SyncStateNames, value);
}

std::optional<RecognitionRecord> RecognitionRecord::restore(const RecognitionRecordData &data) {
    const auto sessionId = PlaybackSessionId::create(data.pluginId, data.sessionKey);
    if (!sessionId.has_value() || !data.candidate.has_value()) return std::nullopt;
    const PlaybackSession session{.id = *sessionId, .userId = data.userId, .deviceId = data.deviceId};
    RecognitionRecord record(session, *data.candidate, data.createdAt);
    record.m_id = data.id;
    record.m_correction = data.correction;
    record.m_status = data.status;
    record.m_diagnostic = data.diagnostic;
    record.m_syncState = data.syncState;
    record.m_syncError = data.syncError;
    record.m_updatedAt = data.updatedAt.isValid() ? data.updatedAt : data.createdAt;
    return record;
}

void RecognitionRecord::markSyncPending(const QDateTime &at) {
    m_syncState = RecognitionSyncState::Pending;
    m_syncError.clear();
    m_updatedAt = at;
}

void RecognitionRecord::markSyncFailed(const QString &safeError, const QDateTime &at) {
    m_syncState = RecognitionSyncState::Failed;
    m_syncError = safeError;
    m_updatedAt = at;
}

void RecognitionRecord::markSynced(const QDateTime &at) {
    m_syncState = RecognitionSyncState::Synced;
    m_syncError.clear();
    m_updatedAt = at;
}
