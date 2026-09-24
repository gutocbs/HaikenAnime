#ifndef HAIKENANIME_IANILISTAUTHPROVIDER_H
#define HAIKENANIME_IANILISTAUTHPROVIDER_H

#include "AniListCredentials.h"

class IAniListAuthProvider {
public:
    virtual ~IAniListAuthProvider() = default;

    /** Returns the credentials that should be used by an AniList request. */
    [[nodiscard]] virtual AniListCredentials credentials() const = 0;
};

#endif // HAIKENANIME_IANILISTAUTHPROVIDER_H
