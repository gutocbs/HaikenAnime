#ifndef HAIKENANIME_ANILISTMEDIAMAPPER_H
#define HAIKENANIME_ANILISTMEDIAMAPPER_H

#include <QJsonObject>

#include "../../domain/anilist/AniListMediaDto.h"
#include "../../domain/media/Media.h"

/** Converts source-specific JSON objects into the common AniList media DTO. */
class AniListMediaMapper final {
public:
    /** Maps one object from the local fixture format. */
    [[nodiscard]] static AniListMediaDto fromFixtureJson(const QJsonObject &object);

    /** Maps one object from the AniList GraphQL media format. */
    [[nodiscard]] static AniListMediaDto FromGraphQlJson(const QJsonObject &object);

    /** Maps the external DTO into the application's media domain model. */
    [[nodiscard]] static Media ToDomainMedia(const AniListMediaDto &media);
};

#endif // HAIKENANIME_ANILISTMEDIAMAPPER_H
