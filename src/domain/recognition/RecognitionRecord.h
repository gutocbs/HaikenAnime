#ifndef HAIKENANIME_RECOGNITIONRECORD_H
#define HAIKENANIME_RECOGNITIONRECORD_H

#include "EpisodeCandidate.h"
#include "PlaybackSession.h"

#include <QDateTime>
#include <QString>

#include <optional>

enum class RecognitionStatus {
    Pending,
    Incomplete,
    MarkedWatched,
    AlreadyWatched,
    Uncertain,
    Ignored,
    Corrected
};

/// Whether a record may move from one status to another. Outcomes are final except for
/// user correction or ignore; nothing returns to Pending.
[[nodiscard]] bool CanTransition(RecognitionStatus from, RecognitionStatus to);

/// History entry for one recognized episode of one playback session.
class RecognitionRecord final {
public:
    [[nodiscard]] static RecognitionRecord create(const PlaybackSession &session,
                                                  const EpisodeCandidate &candidate,
                                                  const QDateTime &createdAt);

    [[nodiscard]] const PlaybackSessionId &sessionId() const { return m_sessionId; }
    [[nodiscard]] const QString &userId() const { return m_userId; }
    [[nodiscard]] const QString &deviceId() const { return m_deviceId; }
    [[nodiscard]] const EpisodeCandidate &candidate() const { return m_candidate; }
    [[nodiscard]] const std::optional<EpisodeCandidate> &correction() const { return m_correction; }
    /// The user's correction when present, otherwise the recognized candidate.
    [[nodiscard]] const EpisodeCandidate &effectiveCandidate() const;
    [[nodiscard]] RecognitionStatus status() const { return m_status; }
    [[nodiscard]] const QDateTime &createdAt() const { return m_createdAt; }
    [[nodiscard]] const QDateTime &updatedAt() const { return m_updatedAt; }
    /// Same session and same episode produce the same key, regardless of confidence.
    [[nodiscard]] QString deduplicationKey() const;

    /// Applies a status change when allowed; returns false and leaves the record untouched otherwise.
    bool transitionTo(RecognitionStatus status, const QDateTime &at);
    bool correct(const EpisodeCandidate &correction, const QDateTime &at);
    bool ignore(const QDateTime &at);

private:
    RecognitionRecord(const PlaybackSession &session, const EpisodeCandidate &candidate,
                      const QDateTime &createdAt);

    PlaybackSessionId m_sessionId;
    QString m_userId;
    QString m_deviceId;
    EpisodeCandidate m_candidate;
    std::optional<EpisodeCandidate> m_correction;
    RecognitionStatus m_status = RecognitionStatus::Pending;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
};

#endif // HAIKENANIME_RECOGNITIONRECORD_H
