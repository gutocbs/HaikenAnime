#ifndef HAIKENANIME_IANILISTAUTHPROVIDER_H
#define HAIKENANIME_IANILISTAUTHPROVIDER_H

#include "../../domain/anilist/AniListCredentials.h"

class IAniListAuthProvider {
public:
    virtual ~IAniListAuthProvider() = default;

    [[nodiscard]] virtual AniListCredentials credentials() const = 0;
};

#endif // HAIKENANIME_IANILISTAUTHPROVIDER_H
