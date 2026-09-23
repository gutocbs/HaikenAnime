#ifndef HAIKENANIME_ANILISTGRAPHQLRESPONSE_H
#define HAIKENANIME_ANILISTGRAPHQLRESPONSE_H

#include <QJsonObject>
#include <QList>

#include "AniListGraphQlError.h"

/** Contains the data and errors returned by an AniList GraphQL operation. */
struct AniListGraphQlResponse {
    QJsonObject data;
    QList<AniListGraphQlError> errors;
    bool dataWasPresent = false;

    /** Returns true when the response contains at least one GraphQL error. */
    [[nodiscard]] bool hasErrors() const {
        return !errors.isEmpty();
    }

    /** Returns true when the response contained a non-null data object. */
    [[nodiscard]] bool hasData() const {
        return dataWasPresent && !data.isEmpty();
    }
};

#endif // HAIKENANIME_ANILISTGRAPHQLRESPONSE_H
