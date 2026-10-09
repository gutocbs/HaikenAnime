#include "AniListOAuthCallbackParser.h"
#include "AniListOAuthConfig.h"

#include <QUrlQuery>

QUrl AniListOAuthConfig::authorizationUrl() const {
    QUrl url(QStringLiteral("https://anilist.co/api/v2/oauth/authorize"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("client_id"), clientId_);
    query.addQueryItem(QStringLiteral("redirect_uri"), redirectUri_.toString(QUrl::FullyEncoded));
    query.addQueryItem(QStringLiteral("response_type"), QStringLiteral("token"));
    url.setQuery(query);
    return url;
}

bool AniListOAuthCallbackParser::parse(const QUrl &callback, AniListCredentials &credentials,
                                       QString &error) const {
    credentials = {};
    error.clear();
    const QUrlQuery fragment(callback.fragment(QUrl::FullyDecoded));
    const auto authorizationError = fragment.queryItemValue(QStringLiteral("error"));
    if (!authorizationError.isEmpty()) {
        auto description = fragment.queryItemValue(QStringLiteral("error_description"));
        description.replace(QLatin1Char('+'), QLatin1Char(' '));
        error = description.isEmpty()
            ? QStringLiteral("AniList authorization was denied.")
            : QStringLiteral("AniList authorization was denied: %1").arg(description);
        return false;
    }

    const auto token = fragment.queryItemValue(QStringLiteral("access_token"));
    if (token.isEmpty()) {
        error = QStringLiteral("AniList OAuth callback does not contain an access token fragment.");
        return false;
    }
    credentials.token = token;
    return true;
}
