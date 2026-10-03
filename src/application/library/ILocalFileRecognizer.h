#ifndef HAIKENANIME_ILOCALFILERECOGNIZER_H
#define HAIKENANIME_ILOCALFILERECOGNIZER_H

#include "LocalFileRecognitionTypes.h"

class ILocalFileRecognizer {
public:
    virtual ~ILocalFileRecognizer() = default;
    virtual LocalFileRecognition recognize(const QString &fileName,
                                           LocalMediaKind requestedKind = LocalMediaKind::Anime) const = 0;
};

#endif
