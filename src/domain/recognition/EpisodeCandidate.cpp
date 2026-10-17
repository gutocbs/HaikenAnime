#include "EpisodeCandidate.h"

EpisodeCandidate::EpisodeCandidate(QString title, const std::optional<int> mediaId,
                                   const std::optional<int> season, const int episode, const double confidence)
    : m_title(std::move(title)), m_mediaId(mediaId), m_season(season), m_episode(episode),
      m_confidence(confidence) {}

std::optional<EpisodeCandidate> EpisodeCandidate::create(const QString &title, const std::optional<int> mediaId,
                                                         const std::optional<int> season, const int episode,
                                                         const double confidence) {
    const auto trimmedTitle = title.trimmed();
    if (episode < 1 || confidence < 0.0 || confidence > 1.0) return std::nullopt;
    if (trimmedTitle.isEmpty() && !mediaId.has_value()) return std::nullopt;
    return EpisodeCandidate(trimmedTitle, mediaId, season, episode, confidence);
}

QString EpisodeCandidate::episodeKey() const {
    const auto series = m_mediaId.has_value() ? QStringLiteral("media:%1").arg(*m_mediaId)
                                              : QStringLiteral("title:%1").arg(m_title.toCaseFolded());
    const auto season = m_season.has_value() ? QString::number(*m_season) : QStringLiteral("-");
    return QStringLiteral("%1|s%2|e%3").arg(series, season).arg(m_episode);
}
