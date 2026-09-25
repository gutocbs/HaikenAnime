#ifndef HAIKENANIME_ANILISTGRAPHQLRESPONSEPARSER_H
#define HAIKENANIME_ANILISTGRAPHQLRESPONSEPARSER_H

#include "AniListGraphQlResponse.h"

#include <QByteArray>
#include <QString>

class AniListGraphQlResponseParser {
public:
    static bool parse(const QByteArray &payload, AniListGraphQlResponse &response, QString &error);
};

#endif // HAIKENANIME_ANILISTGRAPHQLRESPONSEPARSER_H
