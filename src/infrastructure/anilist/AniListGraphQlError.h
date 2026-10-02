#ifndef HAIKENANIME_ANILISTGRAPHQLERROR_H
#define HAIKENANIME_ANILISTGRAPHQLERROR_H

#include <QStringList>

/** Represents one transport error returned inside a valid GraphQL response. */
struct AniListGraphQlError {
    QString message;
    QStringList path;
};

#endif // HAIKENANIME_ANILISTGRAPHQLERROR_H
