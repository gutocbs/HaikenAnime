#ifndef HAIKENANIME_IMEDIAREPOSITORY_H
#define HAIKENANIME_IMEDIAREPOSITORY_H

#include <QList>
#include <QString>

#include "../../domain/media/Media.h"

class IMediaRepository {
public:
    virtual ~IMediaRepository() = default;

    [[nodiscard]] virtual QList<Media> ReadAll(QString &error) = 0;

    /** Inserts or updates the supplied media and reports persistence failures through error. */
    [[nodiscard]] virtual bool Upsert(const QList<Media> &media, QString &error) = 0;
};

#endif // HAIKENANIME_IMEDIAREPOSITORY_H
