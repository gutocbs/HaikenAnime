#ifndef HAIKENANIME_IMEDIAREADER_H
#define HAIKENANIME_IMEDIAREADER_H

#include <QList>
#include <QString>

#include "../../domain/media/Media.h"

/** Provides read access required by media presentation use cases. */
class IMediaReader {
public:
    virtual ~IMediaReader() = default;

    /** Replaces media with all persisted items and clears error on success. */
    [[nodiscard]] virtual bool readAll(QList<Media> &media, QString &error) = 0;
};

#endif // HAIKENANIME_IMEDIAREADER_H
