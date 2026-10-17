#ifndef HAIKENANIME_PLAYBACKOBSERVATIONSERVICE_H
#define HAIKENANIME_PLAYBACKOBSERVATIONSERVICE_H

#include <QDateTime>
#include <QHash>
#include <QString>

#include <chrono>
#include <functional>

#include "../../domain/recognition/EpisodeCandidate.h"
#include "../../domain/recognition/PlaybackSession.h"
#include "../../domain/recognition/RecognitionPolicy.h"

struct ObservationCallbacks final {
    /// Called exactly once per observation when continuous playback reaches the policy threshold.
    std::function<void(const PlaybackSession &, const EpisodeCandidate &)> completed;
    /// Called when an observation ends before the threshold; progress must not change.
    std::function<void(const PlaybackSession &, const EpisodeCandidate &, StopReason)> incomplete;
};

/// Accumulates continuous playback per session from plugin snapshots. Time comes from snapshot
/// timestamps (or the injected clock), never from sleeps, and pauses or gaps do not count.
class PlaybackObservationService final {
public:
    static constexpr std::chrono::seconds DefaultMaximumObservationGap{60};

    PlaybackObservationService(RecognitionPolicy policy, ObservationCallbacks callbacks,
                               std::function<QDateTime()> clock = {});

    void setPolicy(const RecognitionPolicy &policy);

    /// Starts observing; the same episode again keeps progress, a different one replaces it.
    void start(const PlaybackSession &session, const EpisodeCandidate &candidate);
    void update(const PlaybackSessionId &sessionId, const PlaybackSnapshot &snapshot);
    void stop(const PlaybackSessionId &sessionId, StopReason reason);

    [[nodiscard]] bool isObserving(const PlaybackSessionId &sessionId) const;
    [[nodiscard]] std::chrono::seconds accumulatedPlayback(const PlaybackSessionId &sessionId) const;

private:
    struct Observation {
        PlaybackSession session;
        EpisodeCandidate candidate;
        PlaybackState state = PlaybackState::Playing;
        QDateTime lastObservedAt;
        std::chrono::seconds accumulated{0};
        bool completed = false;
    };

    [[nodiscard]] QDateTime now() const;
    void finish(const QString &key, StopReason reason);

    RecognitionPolicy policy_;
    ObservationCallbacks callbacks_;
    std::function<QDateTime()> clock_;
    QHash<QString, Observation> observations_;
};

#endif // HAIKENANIME_PLAYBACKOBSERVATIONSERVICE_H
