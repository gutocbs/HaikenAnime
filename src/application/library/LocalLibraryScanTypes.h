#ifndef HAIKENANIME_LOCALLIBRARYSCANTYPES_H
#define HAIKENANIME_LOCALLIBRARYSCANTYPES_H

#include <QString>
#include <QList>
#include <QStringList>

struct LocalLibraryScanRequest final {
    QString rootPath;
    // Validated, lowercase extensions with a leading period (Task 1 policy).
    QStringList allowedExtensions;
    // Bounds visited directory entries, including filtered entries, from 1 through 200.
    qsizetype batchSize = 200;
    int batchPauseMs = 25;
};

// One request per configured library folder, all sharing the same extension filter.
inline QList<LocalLibraryScanRequest> BuildLocalLibraryScanRequests(const QStringList &rootPaths,
                                                                    const QStringList &allowedExtensions) {
    QList<LocalLibraryScanRequest> requests;
    for (const auto &rootPath : rootPaths) {
        LocalLibraryScanRequest request;
        request.rootPath = rootPath;
        request.allowedExtensions = allowedExtensions;
        requests.append(request);
    }
    return requests;
}

struct LocalLibraryScanProgress final {
    qsizetype visitedEntries = 0;
    qsizetype candidateFiles = 0;
    // Eligible names skipped because their basic metadata disappeared or became invalid.
    qsizetype skippedFiles = 0;
};

struct LocalLibraryScanResult final {
    bool complete = false;
    bool interrupted = false;
    qsizetype candidateFiles = 0;
    QString diagnostic;
    qsizetype skippedFiles = 0;
};

#endif
