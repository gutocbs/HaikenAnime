#ifndef HAIKENANIME_ANILISTOAUTHCALLBACKPARSER_H
#define HAIKENANIME_ANILISTOAUTHCALLBACKPARSER_H

#include <QUrl>

#include "AniListCredentials.h"

/** Parses AniList OAuth implicit-grant callbacks without exposing callback fragments. */
class AniListOAuthCallbackParser final {
public:
    /** Extracts an access token from callback fragment parameters into credentials. */
    [[nodiscard]] bool parse(const QUrl &callback, AniListCredentials &credentials,
                             QString &error) const;
};

#endif // HAIKENANIME_ANILISTOAUTHCALLBACKPARSER_H
