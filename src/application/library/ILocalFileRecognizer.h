#ifndef HAIKENANIME_ILOCALFILERECOGNIZER_H
#define HAIKENANIME_ILOCALFILERECOGNIZER_H

#include "LocalFileRecognitionTypes.h"

class ILocalFileRecognizer {
public:
    virtual ~ILocalFileRecognizer() = default;
    // Local files are always read as anime; manga and novels are never recognized from a filename.
    virtual LocalFileRecognition recognize(const QString &fileName) const = 0;
};

#endif
