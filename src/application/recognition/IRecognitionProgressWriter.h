#ifndef HAIKENANIME_IRECOGNITIONPROGRESSWRITER_H
#define HAIKENANIME_IRECOGNITIONPROGRESSWRITER_H

#include <QString>

#include "../../domain/media/Media.h"

/// Applies an automatic progress change locally and queues it for AniList synchronization.
/// Recognition code never calls AniList directly.
class IRecognitionProgressWriter {
public:
    virtual ~IRecognitionProgressWriter() = default;

    virtual bool apply(const Media &previous, const Media &updated, QString &error) = 0;
};

#endif // HAIKENANIME_IRECOGNITIONPROGRESSWRITER_H
