#ifndef HAIKENANIME_ANILISTGRAPHQLUSERLISTPARSER_H
#define HAIKENANIME_ANILISTGRAPHQLUSERLISTPARSER_H

#include "../../domain/media/Media.h"
#include <QJsonObject>

class AniListGraphQlUserListParser final {
public:
    static bool parse(const QJsonObject &data, QList<Media> &media, QString &error,
                      const QString &listStatus = {});
};

#endif
