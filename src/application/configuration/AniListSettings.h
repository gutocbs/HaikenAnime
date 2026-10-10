#ifndef HAIKENANIME_APPLICATION_ANILISTSETTINGS_H
#define HAIKENANIME_APPLICATION_ANILISTSETTINGS_H

#include <QString>
#include <QUrl>

/** Runtime settings required by the AniList integration. */
struct AniListSettings {
    QString endpoint;
    QString mediaQueryFile;
    QString oauthClientId;
    QUrl oauthRedirectUri;
    int userListPerChunk = 100;
    bool captureGraphQlResponsesForDiagnostics = false;
};

#endif // HAIKENANIME_APPLICATION_ANILISTSETTINGS_H
