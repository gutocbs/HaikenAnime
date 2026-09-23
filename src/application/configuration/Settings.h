#ifndef HAIKENANIME_SETTINGS_H
#define HAIKENANIME_SETTINGS_H

#include "../../domain/anilist/AniListSettings.h"
#include "../../domain/http/HttpSettings.h"

/** Groups application settings without exposing their file format to consumers. */
struct Settings {
    AniListSettings aniList;
    HttpSettings http;
};

#endif // HAIKENANIME_SETTINGS_H
