#ifndef HAIKENANIME_ANILISTMEDIAMAPPER_H
#define HAIKENANIME_ANILISTMEDIAMAPPER_H

#include <QJsonObject>

#include "../../domain/anilist/AniListMediaDto.h"

/** Converts source-specific JSON objects into the common AniList media DTO. */
class AniListMediaMapper final {
public:
    /** Maps one object from the local fixture format. */
    [[nodiscard]] static AniListMediaDto fromFixtureJson(const QJsonObject &object);

    /** Maps one object from the AniList GraphQL media format. */
    [[nodiscard]] static AniListMediaDto fromGraphQlJson(const QJsonObject &object);
};

#endif // HAIKENANIME_ANILISTMEDIAMAPPER_H
