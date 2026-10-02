#ifndef HAIKENANIME_ANILISTGRAPHQLPAGEPARSER_H
#define HAIKENANIME_ANILISTGRAPHQLPAGEPARSER_H

#include "../../application/media/MediaPage.h"

#include <QJsonObject>
#include <QString>

class AniListGraphQlPageParser {
public:
    static bool parse(const QJsonObject &data, MediaPage &page, QString &error);
};

#endif // HAIKENANIME_ANILISTGRAPHQLPAGEPARSER_H
