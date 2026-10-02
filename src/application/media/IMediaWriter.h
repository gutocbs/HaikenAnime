#ifndef HAIKENANIME_IMEDIAWRITER_H
#define HAIKENANIME_IMEDIAWRITER_H

#include <QList>
#include <QString>

#include "../../domain/media/Media.h"

/** Provides write access required by media synchronization use cases. */
class IMediaWriter {
public:
    virtual ~IMediaWriter() = default;

    /** Inserts or updates media and clears error on success. */
    [[nodiscard]] virtual bool upsert(const QList<Media> &media, QString &error) = 0;
};

#endif // HAIKENANIME_IMEDIAWRITER_H
