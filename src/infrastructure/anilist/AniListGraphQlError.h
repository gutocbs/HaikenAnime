#ifndef HAIKENANIME_ANILISTGRAPHQLERROR_H
#define HAIKENANIME_ANILISTGRAPHQLERROR_H

#include <QStringList>

/** Represents one transport error returned inside a valid GraphQL response. */
struct AniListGraphQlError {
    QString message;
    QStringList path;
    /** GraphQL error status code (for example 404); 0 when absent. */
    int status = 0;
};

#endif // HAIKENANIME_ANILISTGRAPHQLERROR_H
