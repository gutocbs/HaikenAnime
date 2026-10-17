#ifndef HAIKENANIME_EPISODECANDIDATE_H
#define HAIKENANIME_EPISODECANDIDATE_H

#include <QString>

#include <optional>

/// Episode a playback session is believed to show, with the recognizer's confidence (0..1).
class EpisodeCandidate final {
public:
    /// Returns a candidate only when the episode is >= 1, the confidence is within [0, 1]
    /// and either a non-blank title or a media id identifies the series.
    [[nodiscard]] static std::optional<EpisodeCandidate> create(const QString &title,
                                                                std::optional<int> mediaId,
                                                                std::optional<int> season,
                                                                int episode,
                                                                double confidence);

    [[nodiscard]] const QString &title() const { return m_title; }
    [[nodiscard]] std::optional<int> mediaId() const { return m_mediaId; }
    [[nodiscard]] std::optional<int> season() const { return m_season; }
    [[nodiscard]] int episode() const { return m_episode; }
    [[nodiscard]] double confidence() const { return m_confidence; }
    /// Identifies the episode independently of confidence: media id when known,
    /// otherwise the case-folded title.
    [[nodiscard]] QString episodeKey() const;

private:
    EpisodeCandidate(QString title, std::optional<int> mediaId, std::optional<int> season, int episode,
                     double confidence);

    QString m_title;
    std::optional<int> m_mediaId;
    std::optional<int> m_season;
    int m_episode = 1;
    double m_confidence = 0.0;
};

#endif // HAIKENANIME_EPISODECANDIDATE_H
