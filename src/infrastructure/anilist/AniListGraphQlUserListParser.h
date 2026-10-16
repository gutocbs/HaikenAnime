#ifndef HAIKENANIME_ANILISTGRAPHQLUSERLISTPARSER_H
#define HAIKENANIME_ANILISTGRAPHQLUSERLISTPARSER_H

#include "../../domain/media/Media.h"
#include <QJsonObject>
#include <QSet>

class AniListGraphQlUserListParser final {
public:
    static bool parse(const QJsonObject &data, QList<Media> &media, QString &error,
                      const QStringList &acceptedListStatuses = {},
                      const QList<MediaType> &acceptedMediaTypes = {},
                      QSet<int> *observedMediaIds = nullptr);
};

#endif
