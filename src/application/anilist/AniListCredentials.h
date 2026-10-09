#ifndef HAIKENANIME_APPLICATION_ANILISTCREDENTIALS_H
#define HAIKENANIME_APPLICATION_ANILISTCREDENTIALS_H

#include <QString>

#include <QtGlobal>

struct AniListCredentials {
    QString username;
    QString token;
    qint64 userId = 0;
    qint64 expiresAtUnixSeconds = 0;
};

#endif // HAIKENANIME_APPLICATION_ANILISTCREDENTIALS_H
