#ifndef HAIKENANIME_LOCALFILERECORD_H
#define HAIKENANIME_LOCALFILERECORD_H

#include <QDateTime>
#include <QString>
#include <optional>

struct LocalFileObservation final {
    QString rootPath;
    QString relativePath;
    QString normalizedRelativePath;
    QString fileName;
    QString extension;
    qint64 sizeBytes = 0;
    QDateTime modifiedAt;
};

enum class LibraryScanStatus { Running, Succeeded, Failed, Interrupted };

struct LocalFileRecognitionRecord final {
    qint64 id = 0;
    QString rootPath;
    QString relativePath;
    QString normalizedRelativePath;
    QString fileName;
    qint64 sizeBytes = 0;
    QDateTime modifiedAt;
    bool available = false;
    QString recognitionState;
    QString extractedTitle;
    QString mediaKind;
    std::optional<int> season;
    std::optional<int> episode;
    std::optional<int> mediaId;
    QString diagnostic;
};

#endif
