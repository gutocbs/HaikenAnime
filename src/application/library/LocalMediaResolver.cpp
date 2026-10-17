#include "LocalMediaResolver.h"

LocalMediaMatch LocalMediaResolver::resolve(const LocalFileRecognition &recognition,
                                            const QList<Media> &catalog) const {
    return resolve(recognition, LocalMediaCatalogIndex(catalog));
}

LocalMediaMatch LocalMediaResolver::resolve(const LocalFileRecognition &recognition,
                                            const LocalMediaCatalogIndex &index) const {
    LocalMediaMatch result;
    if (recognition.mediaKind != LocalMediaKind::Anime) {
        result.state = LocalRecognitionState::Unsupported;
        result.diagnostic = QStringLiteral("The local media kind is not supported yet.");
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
    if (!recognition.episode.has_value() || recognition.episode.value() <= 0) {
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
    const auto matches = index.find(title);
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
