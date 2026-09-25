#ifndef HAIKENANIME_COVERCACHEENTRY_H
#define HAIKENANIME_COVERCACHEENTRY_H

#include <QDateTime>
#include <QString>

#include "CoverQuality.h"

struct CoverCacheEntry {
    int mediaId = 0;
    QString remoteUrl;
    CoverQuality quality = CoverQuality::Medium;
    QString relativePath;
    QString mimeType;
    qint64 byteSize = 0;
    QString etag;
    QString lastModified;
    QDateTime validatedAt;
};

#endif // HAIKENANIME_COVERCACHEENTRY_H
