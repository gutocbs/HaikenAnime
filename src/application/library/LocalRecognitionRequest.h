#ifndef HAIKENANIME_LOCALRECOGNITIONREQUEST_H
#define HAIKENANIME_LOCALRECOGNITIONREQUEST_H

#include "../../domain/media/Media.h"

#include <QString>
#include <QStringList>

// What a saved media edit asks of the local recognition. Every save of a media with a folder
// re-evaluates that folder (idempotent; repairs associations that survive rescans), a moved or
// cleared folder also re-evaluates the old one so its files can drop the stale association, and
// changed alternative names re-evaluate the pending files.
struct LocalRecognitionRequest final {
    QStringList folderPaths;
    bool alternativeNamesChanged = false;

    bool isRequired() const { return !folderPaths.isEmpty() || alternativeNamesChanged; }

    static LocalRecognitionRequest Between(const Media &previous, const Media &saved) {
        LocalRecognitionRequest request;
        const auto savedPath = saved.LocalPath.trimmed();
        const auto previousPath = previous.LocalPath.trimmed();
        if (!savedPath.isEmpty()) request.folderPaths.append(savedPath);
        if (!previousPath.isEmpty() && previousPath != savedPath) request.folderPaths.append(previousPath);
        request.alternativeNamesChanged = saved.AlternativeNames != previous.AlternativeNames;
        return request;
    }
};

#endif // HAIKENANIME_LOCALRECOGNITIONREQUEST_H
