#include "AnitomyLocalFileRecognizer.h"

#include <iostream>

#include "../../../lib/anitomy/anitomy.h"

#include <QFileInfo>
#include <QRegularExpression>

namespace {
std::optional<int> ParseNumber(const std::wstring &value) {
    if (value.empty()) return std::nullopt;
    bool ok = false;
    const auto number = QString::fromStdWString(value).toInt(&ok);
    return ok && number >= 0 ? std::optional<int>(number) : std::nullopt;
}

bool ContainsUnsupportedEpisodeSyntax(const std::wstring &value) {
    const auto text = QString::fromStdWString(value);
    return text.contains(QLatin1Char('-')) || text.contains(QLatin1Char(','))
        || text.contains(QLatin1Char('&')) || text.contains(QLatin1Char('/'));
}

bool ContainsEpisodeRangeOrList(const QString &fileName) {
    static const QRegularExpression expression(QStringLiteral(R"(\b\d+\s*[-&,/]\s*\d+\b)"));
    return expression.match(fileName).hasMatch();
}

QString StripBracketGroups(QString name) {
    static const QRegularExpression groups(QStringLiteral(R"(\[[^\]]*\]|\([^)]*\))"));
    name.remove(groups);
    return name.simplified();
}

QString NameWithoutExtension(const QString &fileName) {
    const auto name = QFileInfo(fileName).fileName();
    const auto dot = name.lastIndexOf(QLatin1Char('.'));
    return dot > 0 && name.size() - dot <= 5 ? name.left(dot) : name;
}

std::optional<int> FindFallbackSeason(const QString &cleanName, QRegularExpressionMatch *matchOut) {
    static const QList<QRegularExpression> patterns{
        QRegularExpression(QStringLiteral(R"(\bSeason\s*(\d{1,2})\b)"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral(R"(\b(\d{1,2})(?:st|nd|rd|th)\s+Season\b)"),
                           QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral(R"(\bS(\d{1,2})(?=E\d|\b))"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral(R"(\bPart\s*(\d{1,2})\b)"), QRegularExpression::CaseInsensitiveOption)};
    for (const auto &pattern : patterns) {
        const auto match = pattern.match(cleanName);
        if (!match.hasMatch()) continue;
        const int season = match.captured(1).toInt();
        if (season < 1) continue;
        *matchOut = match;
        return season;
    }
    return std::nullopt;
}

bool IsPlausibleEpisode(const int episode) {
    const bool looksLikeYear = episode >= 1900 && episode <= 2099;
    return episode >= 1 && episode <= 2000 && !looksLikeYear;
}

std::optional<int> FindFallbackEpisode(const QString &cleanName) {
    static const QList<QRegularExpression> patterns{
        QRegularExpression(QStringLiteral(R"(\bS\d{1,2}E(\d{1,4})\b)"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral(R"(\s[-\x{2013}]\s*(\d{1,4})(?:v\d)?\b)")),
        QRegularExpression(QStringLiteral(R"(\b(?:EP?|Episode)\s*(\d{1,4})\b)"),
                           QRegularExpression::CaseInsensitiveOption)};
    for (const auto &pattern : patterns) {
        const auto match = pattern.match(cleanName);
        if (!match.hasMatch()) continue;
        const int episode = match.captured(1).toInt();
        if (IsPlausibleEpisode(episode)) return episode;
    }
    return std::nullopt;
}

// Removes the season marker (e.g. "Season 2") from the title when Anitomy left it there.
QString RemoveSeasonMarker(const QString &title, const QRegularExpressionMatch &fileMatch) {
    const QRegularExpression marker(QStringLiteral("\\s*") + QRegularExpression::escape(fileMatch.captured(0)),
                                    QRegularExpression::CaseInsensitiveOption);
    QString cleaned = title;
    cleaned.remove(marker);
    return cleaned.simplified();
}
}

LocalFileRecognition AnitomyLocalFileRecognizer::recognize(const QString &fileName,
                                                           const LocalMediaKind requestedKind) const {
    LocalFileRecognition result;
    result.mediaKind = requestedKind;
    if (requestedKind != LocalMediaKind::Anime) {
        result.state = LocalRecognitionState::Unsupported;
        result.diagnostic = QStringLiteral("The requested local media kind is not supported yet.");
        return result;
    }
    if (fileName.trimmed().isEmpty()) {
        result.mediaKind = LocalMediaKind::Anime;
        result.diagnostic = QStringLiteral("The local filename is empty.");
        return result;
    }

    anitomy::Anitomy parser;
    if (!parser.Parse(QFileInfo(fileName).fileName().toStdWString())) {
        result.mediaKind = LocalMediaKind::Anime;
        result.diagnostic = QStringLiteral("Anitomy could not parse the local filename.");
        return result;
    }
    const auto &elements = parser.elements();
    result.extractedTitle = QString::fromStdWString(elements.get(anitomy::kElementAnimeTitle)).trimmed();
    const auto episodeValue = elements.get(anitomy::kElementEpisodeNumber);
    const auto alternateEpisodeValue = elements.get(anitomy::kElementEpisodeNumberAlt);
    const auto seasonValue = elements.get(anitomy::kElementAnimeSeason);
    const auto episodeValues = elements.get_all(anitomy::kElementEpisodeNumber);

    if (result.extractedTitle.isEmpty()) {
        result.diagnostic = QStringLiteral("No anime title was extracted from the local filename.");
        return result;
    }

    // The fallback only fills what Anitomy left empty; a valid Anitomy value is never overridden.
    const auto cleanName = StripBracketGroups(NameWithoutExtension(fileName));
    bool usedFallback = false;
    QString rangeCheckName = fileName;
    std::optional<int> season = ParseNumber(seasonValue);
    // A "Season N - 01" marker must not be read as the episode range "N - 01", even when Anitomy found the season.
    QRegularExpressionMatch seasonMatch;
    const auto markerSeason = FindFallbackSeason(cleanName, &seasonMatch);
    if (markerSeason.has_value()) rangeCheckName.remove(seasonMatch.captured(0), Qt::CaseInsensitive);
    if (!season.has_value() && markerSeason.has_value()) {
        season = markerSeason;
        result.extractedTitle = RemoveSeasonMarker(result.extractedTitle, seasonMatch);
        usedFallback = true;
    }
    if (episodeValue.empty() && alternateEpisodeValue.empty()) {
        result.episode = FindFallbackEpisode(cleanName);
        if (!result.episode.has_value()) {
            result.diagnostic = QStringLiteral("No numeric episode was extracted from the local filename.");
            return result;
        }
        usedFallback = true;
    } else {
        if (episodeValues.size() > 1 || ContainsEpisodeRangeOrList(rangeCheckName)
            || ContainsUnsupportedEpisodeSyntax(episodeValue)
            || ContainsUnsupportedEpisodeSyntax(alternateEpisodeValue)) {
            result.state = LocalRecognitionState::Ambiguous;
            result.diagnostic = QStringLiteral("The filename contains an episode range or multiple episode values.");
            return result;
        }
        result.episode = ParseNumber(!episodeValue.empty() ? episodeValue : alternateEpisodeValue);
        if (!result.episode.has_value()) {
            result.state = LocalRecognitionState::Ambiguous;
            result.diagnostic = QStringLiteral("The extracted episode is not a single numeric value.");
            return result;
        }
    }
    result.season = season;
    if (usedFallback) result.diagnostic = QStringLiteral("season/episode from fallback");
    result.state = LocalRecognitionState::Recognized;
    return result;
}
