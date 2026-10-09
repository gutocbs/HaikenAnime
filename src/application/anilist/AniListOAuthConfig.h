#ifndef HAIKENANIME_ANILISTOAUTHCONFIG_H
#define HAIKENANIME_ANILISTOAUTHCONFIG_H

#include <QUrl>

#include <utility>

/** Holds the registered AniList OAuth client and callback configuration. */
class AniListOAuthConfig final {
public:
    AniListOAuthConfig(QString clientId, QUrl redirectUri)
        : clientId_(std::move(clientId)), redirectUri_(std::move(redirectUri)) {
    }

    /** Returns the AniList OAuth implicit-grant authorization URL. */
    [[nodiscard]] QUrl authorizationUrl() const;

private:
    QString clientId_;
    QUrl redirectUri_;
};

#endif // HAIKENANIME_ANILISTOAUTHCONFIG_H
