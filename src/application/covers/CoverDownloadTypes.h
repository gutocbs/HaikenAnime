#ifndef HAIKENANIME_COVERDOWNLOADTYPES_H
#define HAIKENANIME_COVERDOWNLOADTYPES_H

#include "CoverQuality.h"
#include <QString>
#include <QUrl>

enum class CoverPriority { Visible, Prefetch };
enum class CoverState { Missing, Queued, Downloading, Available, Failed };
enum class CoverFailureCategory { Transport, HttpTemporary, HttpPermanent, InvalidContent, FileSystem, Persistence, Cancelled };

struct CoverRequest {
    int mediaId = 0;
    QUrl remoteUrl;
    CoverQuality quality = CoverQuality::Medium;
    CoverPriority priority = CoverPriority::Prefetch;
    quint64 generation = 0;
};

struct CoverDownloadResult {
    CoverRequest request;
    bool succeeded = false;
    QString temporaryPath;
    QString mimeType;
    QString etag;
    QString lastModified;
    CoverFailureCategory failure = CoverFailureCategory::Transport;
    int httpStatus = 0;
    int retryAfterMs = 0;
    QString error;
};

Q_DECLARE_METATYPE(CoverState)
#endif
