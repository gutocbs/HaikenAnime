#ifndef HAIKENANIME_ANILISTSETTINGS_H
#define HAIKENANIME_ANILISTSETTINGS_H

#include <QString>

/** Runtime settings required by the AniList integration. */
struct AniListSettings {
    QString endpoint;
    QString mediaQueryFile;
};

#endif // HAIKENANIME_ANILISTSETTINGS_H
