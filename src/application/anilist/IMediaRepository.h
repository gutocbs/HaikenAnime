#ifndef HAIKENANIME_IMEDIAREPOSITORY_H
#define HAIKENANIME_IMEDIAREPOSITORY_H

#include <QList>
#include <QString>

#include "../../domain/media/Media.h"

class IMediaRepository {
public:
    virtual ~IMediaRepository() = default;

    [[nodiscard]] virtual bool upsert(const QList<Media> &media, QString &error) = 0;
};

#endif // HAIKENANIME_IMEDIAREPOSITORY_H
