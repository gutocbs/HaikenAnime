#include "RecognitionPolicy.h"

RecognitionPolicy::RecognitionPolicy(const int minimumPlaybackSeconds, const double minimumConfidence)
    : m_minimumPlaybackSeconds(minimumPlaybackSeconds), m_minimumConfidence(minimumConfidence) {}

std::optional<RecognitionPolicy> RecognitionPolicy::create(const int minimumPlaybackSeconds,
                                                           const double minimumConfidence) {
    if (minimumPlaybackSeconds <= 0) return std::nullopt;
    if (minimumConfidence < 0.0 || minimumConfidence > 1.0) return std::nullopt;
    return RecognitionPolicy(minimumPlaybackSeconds, minimumConfidence);
}

bool RecognitionPolicy::hasReachedThreshold(const std::chrono::seconds continuousPlayback) const {
    return continuousPlayback.count() >= m_minimumPlaybackSeconds;
}

bool RecognitionPolicy::isConfident(const EpisodeCandidate &candidate) const {
    return candidate.confidence() >= m_minimumConfidence;
}
