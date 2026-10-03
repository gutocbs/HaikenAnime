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
}

LocalFileRecognition AnitomyLocalFileRecognizer::recognize(const QString &fileName,
                                                           const LocalMediaKind requestedKind) const {
    if (fileName.contains("Arakawa Under the Bridge_-_02"))
        std::cout << "Arakawa" << std::endl;
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
    if (episodeValue.empty() && alternateEpisodeValue.empty()) {
        result.diagnostic = QStringLiteral("No numeric episode was extracted from the local filename.");
        return result;
    }
    if (episodeValues.size() > 1 || ContainsEpisodeRangeOrList(fileName)
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
    result.season = ParseNumber(seasonValue);
    result.state = LocalRecognitionState::Recognized;
    return result;
}
