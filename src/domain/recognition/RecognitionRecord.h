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

/// AniList synchronization state of the local progress change made by a recognition.
enum class RecognitionSyncState { NotRequired, Pending, Failed, Synced };

/// Stable persisted names (kebab-case).
[[nodiscard]] QString ToString(RecognitionStatus status);
[[nodiscard]] QString ToString(RecognitionSyncState state);
[[nodiscard]] std::optional<RecognitionStatus> RecognitionStatusFromString(const QString &value);
[[nodiscard]] std::optional<RecognitionSyncState> RecognitionSyncStateFromString(const QString &value);

/// Flat stored form used by repositories to rebuild a record.
struct RecognitionRecordData final {
    qint64 id = 0;
    QString pluginId;
    QString sessionKey;
    QString userId;
    QString deviceId;
    std::optional<EpisodeCandidate> candidate;
    std::optional<EpisodeCandidate> correction;
    RecognitionStatus status = RecognitionStatus::Pending;
    QString diagnostic;
    RecognitionSyncState syncState = RecognitionSyncState::NotRequired;
    QString syncError;
    QDateTime createdAt;
    QDateTime updatedAt;
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
    /// Rebuilds a stored record; returns std::nullopt when identity or candidate is invalid.
    [[nodiscard]] static std::optional<RecognitionRecord> restore(const RecognitionRecordData &data);

    [[nodiscard]] qint64 id() const { return m_id; }
    void assignId(qint64 id) { m_id = id; }

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
    /// Safe, value-free explanation of the outcome (e.g. "low-confidence").
    [[nodiscard]] const QString &diagnostic() const { return m_diagnostic; }
    void setDiagnostic(const QString &diagnostic) { m_diagnostic = diagnostic; }
    [[nodiscard]] RecognitionSyncState syncState() const { return m_syncState; }
    [[nodiscard]] const QString &syncError() const { return m_syncError; }
    void markSyncPending(const QDateTime &at);
    void markSyncFailed(const QString &safeError, const QDateTime &at);
    void markSynced(const QDateTime &at);

    /// Applies a status change when allowed; returns false and leaves the record untouched otherwise.
    bool transitionTo(RecognitionStatus status, const QDateTime &at);
    bool correct(const EpisodeCandidate &correction, const QDateTime &at);
    bool ignore(const QDateTime &at);

private:
    RecognitionRecord(const PlaybackSession &session, const EpisodeCandidate &candidate,
                      const QDateTime &createdAt);

    qint64 m_id = 0;
    PlaybackSessionId m_sessionId;
    QString m_userId;
    QString m_deviceId;
    EpisodeCandidate m_candidate;
    std::optional<EpisodeCandidate> m_correction;
    RecognitionStatus m_status = RecognitionStatus::Pending;
    QString m_diagnostic;
    RecognitionSyncState m_syncState = RecognitionSyncState::NotRequired;
    QString m_syncError;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
};

#endif // HAIKENANIME_RECOGNITIONRECORD_H
