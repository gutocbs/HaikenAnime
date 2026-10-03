#ifndef HAIKENANIME_LOCALFILERECOGNITIONTYPES_H
#define HAIKENANIME_LOCALFILERECOGNITIONTYPES_H

#include <QString>

#include <optional>

enum class LocalMediaKind {
    Anime,
    Manga,
    Novel,
    Unsupported
};

enum class LocalRecognitionState {
    Unprocessed,
    Recognized,
    Unrecognized,
    Ambiguous,
    Unsupported,
    Associated
};

struct LocalFileRecognition final {
    LocalRecognitionState state = LocalRecognitionState::Unrecognized;
    LocalMediaKind mediaKind = LocalMediaKind::Unsupported;
    QString extractedTitle;
    std::optional<int> season;
    std::optional<int> episode;
    QString diagnostic;
};

#endif
