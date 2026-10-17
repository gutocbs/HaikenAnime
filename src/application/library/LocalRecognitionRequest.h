#ifndef HAIKENANIME_LOCALRECOGNITIONREQUEST_H
#define HAIKENANIME_LOCALRECOGNITIONREQUEST_H

#include "../../domain/media/Media.h"

#include <QString>

// What a saved media edit asks of the local recognition: re-evaluate the files under a new folder,
// and/or re-evaluate the pending files because the media gained or lost alternative names.
struct LocalRecognitionRequest final {
    QString folderPath;
    bool alternativeNamesChanged = false;

    bool isRequired() const { return !folderPath.isEmpty() || alternativeNamesChanged; }

    static LocalRecognitionRequest Between(const Media &previous, const Media &saved) {
        LocalRecognitionRequest request;
        const auto savedPath = saved.LocalPath.trimmed();
        if (savedPath != previous.LocalPath.trimmed()) request.folderPath = savedPath;
        request.alternativeNamesChanged = saved.AlternativeNames != previous.AlternativeNames;
        return request;
    }
};

#endif // HAIKENANIME_LOCALRECOGNITIONREQUEST_H
