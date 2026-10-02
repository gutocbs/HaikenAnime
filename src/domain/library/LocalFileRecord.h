#ifndef HAIKENANIME_LOCALFILERECORD_H
#define HAIKENANIME_LOCALFILERECORD_H

#include <QDateTime>
#include <QString>

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

#endif
