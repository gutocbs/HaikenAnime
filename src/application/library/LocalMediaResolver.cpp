#include "LocalMediaResolver.h"

namespace {
QString OrdinalFor(const int number) {
    const auto lastTwoDigits = number % 100;
    const auto lastDigit = number % 10;
    if (lastTwoDigits >= 11 && lastTwoDigits <= 13) return QStringLiteral("%1th").arg(number);
    if (lastDigit == 1) return QStringLiteral("%1st").arg(number);
    if (lastDigit == 2) return QStringLiteral("%1nd").arg(number);
    if (lastDigit == 3) return QStringLiteral("%1rd").arg(number);
    return QStringLiteral("%1th").arg(number);
}

QString RomanFor(const int number) {
    static const QStringList numerals{QStringLiteral("ii"), QStringLiteral("iii"), QStringLiteral("iv"),
                                      QStringLiteral("v"), QStringLiteral("vi"), QStringLiteral("vii"),
                                      QStringLiteral("viii"), QStringLiteral("ix"), QStringLiteral("x")};
    return number >= 2 && number <= 10 ? numerals.at(number - 2) : QString();
}

// Season markers a catalog title can carry after the base title, already normalized, in priority order.
QStringList SeasonSuffixes(const int season) {
    QStringList suffixes{QStringLiteral("%1 season").arg(OrdinalFor(season)),
                         QStringLiteral("season %1").arg(season),
                         QStringLiteral("s%1").arg(season),
                         QStringLiteral("part %1").arg(season)};
    if (!RomanFor(season).isEmpty()) suffixes.append(RomanFor(season));
    suffixes.append(QString::number(season));
    return suffixes;
}

bool TitleEndsWithSeasonMarker(const QString &normalizedTitle, const int season) {
    for (const auto &suffix : SeasonSuffixes(season)) {
        if (normalizedTitle.endsWith(QLatin1Char(' ') + suffix)) return true;
    }
    return false;
}
}

LocalMediaMatch LocalMediaResolver::resolve(const LocalFileRecognition &recognition,
                                            const QList<Media> &catalog) const {
    return resolve(recognition, LocalMediaCatalogIndex(catalog));
}

LocalMediaMatch LocalMediaResolver::resolve(const LocalFileRecognition &recognition,
                                            const LocalMediaCatalogIndex &index) const {
    return resolve(recognition, index, QString());
}

LocalMediaMatch LocalMediaResolver::resolve(const LocalFileRecognition &recognition,
                                            const LocalMediaCatalogIndex &index,
                                            const QString &absoluteFilePath) const {
    LocalMediaMatch result;
    if (recognition.mediaKind != LocalMediaKind::Anime) {
        result.state = LocalRecognitionState::Unsupported;
        result.diagnostic = QStringLiteral("The local media kind is not supported yet.");
        return result;
    }
    const auto hasValidEpisode = recognition.episode.has_value() && recognition.episode.value() > 0;
    const auto folderMatches = absoluteFilePath.isEmpty() ? QList<int>() : index.findByFolder(absoluteFilePath);
    if (!folderMatches.isEmpty()) {
        if (!hasValidEpisode) {
            result.state = LocalRecognitionState::Ambiguous;
            result.diagnostic = QStringLiteral("The recognized local file does not contain one valid episode number.");
        } else if (folderMatches.size() > 1) {
            result.state = LocalRecognitionState::Ambiguous;
            result.diagnostic = QStringLiteral("Multiple catalog media share the folder of the local file.");
        } else {
            result.state = LocalRecognitionState::Associated;
            result.mediaId = folderMatches.first();
        }
        return result;
    }
    if (recognition.state != LocalRecognitionState::Recognized) {
        result.state = recognition.state == LocalRecognitionState::Ambiguous
            ? LocalRecognitionState::Ambiguous
            : recognition.state == LocalRecognitionState::Unsupported
                ? LocalRecognitionState::Unsupported
                : LocalRecognitionState::Unrecognized;
        result.diagnostic = QStringLiteral("The local recognition result is not eligible for association.");
        return result;
    }
    if (!hasValidEpisode) {
        result.state = LocalRecognitionState::Ambiguous;
        result.diagnostic = QStringLiteral("The recognized local file does not contain one valid episode number.");
        return result;
    }

    const auto title = LocalMediaCatalogIndex::NormalizeTitle(recognition.extractedTitle);
    if (title.isEmpty()) {
        result.state = LocalRecognitionState::Unrecognized;
        result.diagnostic = QStringLiteral("The recognized local title is empty.");
        return result;
    }
    QList<int> matches;
    const auto season = recognition.season.value_or(1);
    if (season > 1 && !TitleEndsWithSeasonMarker(title, season)) {
        // A later season never falls back to the plain title, which belongs to the first season.
        // The first key that matches anything decides; one media wins, several are ambiguous.
        for (const auto &suffix : SeasonSuffixes(season)) {
            matches = index.find(LocalMediaCatalogIndex::NormalizeTitle(title + QLatin1Char(' ') + suffix));
            if (!matches.isEmpty()) break;
        }
        if (matches.isEmpty()) {
            result.state = LocalRecognitionState::Ambiguous;
            result.diagnostic = QStringLiteral("No catalog media matches the extracted title for season %1.").arg(season);
            return result;
        }
    } else {
        matches = index.find(title);
    }
    if (matches.isEmpty()) {
        result.state = LocalRecognitionState::Unrecognized;
        result.diagnostic = QStringLiteral("No catalog media matches the extracted local title.");
        return result;
    }
    if (matches.size() != 1) {
        result.state = LocalRecognitionState::Ambiguous;
        result.diagnostic = QStringLiteral("Multiple catalog media match the extracted local title.");
        return result;
    }
    result.state = LocalRecognitionState::Associated;
    result.mediaId = matches.first();
    return result;
}
