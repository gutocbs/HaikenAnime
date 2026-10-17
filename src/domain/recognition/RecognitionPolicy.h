#ifndef HAIKENANIME_RECOGNITIONPOLICY_H
#define HAIKENANIME_RECOGNITIONPOLICY_H

#include "EpisodeCandidate.h"

#include <chrono>
#include <optional>

/// User-configurable rules deciding when an observed playback counts as watched.
class RecognitionPolicy final {
public:
    static constexpr int DefaultMinimumPlaybackSeconds = 180;
    static constexpr double DefaultMinimumConfidence = 0.8;

    RecognitionPolicy() = default;

    /// Returns a policy only for a positive duration and a confidence within [0, 1].
    [[nodiscard]] static std::optional<RecognitionPolicy> create(
        int minimumPlaybackSeconds, double minimumConfidence = DefaultMinimumConfidence);

    [[nodiscard]] int minimumPlaybackSeconds() const { return m_minimumPlaybackSeconds; }
    [[nodiscard]] double minimumConfidence() const { return m_minimumConfidence; }
    [[nodiscard]] bool hasReachedThreshold(std::chrono::seconds continuousPlayback) const;
    [[nodiscard]] bool isConfident(const EpisodeCandidate &candidate) const;

private:
    RecognitionPolicy(int minimumPlaybackSeconds, double minimumConfidence);

    int m_minimumPlaybackSeconds = DefaultMinimumPlaybackSeconds;
    double m_minimumConfidence = DefaultMinimumConfidence;
};

#endif // HAIKENANIME_RECOGNITIONPOLICY_H
