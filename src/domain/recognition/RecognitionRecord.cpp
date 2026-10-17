#include "RecognitionRecord.h"

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
