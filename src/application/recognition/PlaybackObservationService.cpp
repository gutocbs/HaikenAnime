#include "PlaybackObservationService.h"

#include <algorithm>
#include <utility>

PlaybackObservationService::PlaybackObservationService(RecognitionPolicy policy, ObservationCallbacks callbacks,
                                                       std::function<QDateTime()> clock)
    : policy_(policy), callbacks_(std::move(callbacks)), clock_(std::move(clock)) {}

void PlaybackObservationService::setPolicy(const RecognitionPolicy &policy) {
    policy_ = policy;
}

void PlaybackObservationService::start(const PlaybackSession &session, const EpisodeCandidate &candidate) {
    const auto key = session.id.key();
    QDateTime reference = session.startedAt.isValid() ? session.startedAt : now();
    const auto existing = observations_.constFind(key);
    if (existing != observations_.cend()) {
        if (existing->candidate.episodeKey() == candidate.episodeKey()) return;
        // Same player session moved to another episode (Plex autoplay, mpv playlist).
        reference = existing->lastObservedAt;
        finish(key, StopReason::Replaced);
    }
    observations_.insert(key, Observation{.session = session, .candidate = candidate, .lastObservedAt = reference});
}

void PlaybackObservationService::update(const PlaybackSessionId &sessionId, const PlaybackSnapshot &snapshot) {
    const auto key = sessionId.key();
    const auto found = observations_.find(key);
    if (found == observations_.end()) return;
    if (!snapshot.available) {
        finish(key, StopReason::Unavailable);
        return;
    }

    const auto observedAt = snapshot.observedAt.isValid() ? snapshot.observedAt : now();
    if (found->state == PlaybackState::Playing && observedAt > found->lastObservedAt) {
        const auto elapsed = std::chrono::seconds(found->lastObservedAt.secsTo(observedAt));
        // A long silence (sleep, lost IPC, slow poll) is not evidence of continuous playback.
        found->accumulated += std::min(elapsed, DefaultMaximumObservationGap);
    }
    if (observedAt > found->lastObservedAt) found->lastObservedAt = observedAt;
    found->state = snapshot.state;

    if (!found->completed && policy_.hasReachedThreshold(found->accumulated)) {
        found->completed = true;
        if (callbacks_.completed) callbacks_.completed(found->session, found->candidate);
    }
}

void PlaybackObservationService::stop(const PlaybackSessionId &sessionId, const StopReason reason) {
    finish(sessionId.key(), reason);
}

bool PlaybackObservationService::isObserving(const PlaybackSessionId &sessionId) const {
    return observations_.contains(sessionId.key());
}

std::chrono::seconds PlaybackObservationService::accumulatedPlayback(const PlaybackSessionId &sessionId) const {
    const auto found = observations_.constFind(sessionId.key());
    return found == observations_.cend() ? std::chrono::seconds::zero() : found->accumulated;
}

QDateTime PlaybackObservationService::now() const {
    return clock_ ? clock_() : QDateTime::currentDateTimeUtc();
}

void PlaybackObservationService::finish(const QString &key, const StopReason reason) {
    const auto found = observations_.find(key);
    if (found == observations_.end()) return;
    const auto observation = *found;
    observations_.erase(found);
    if (!observation.completed && callbacks_.incomplete) {
        callbacks_.incomplete(observation.session, observation.candidate, reason);
    }
}
