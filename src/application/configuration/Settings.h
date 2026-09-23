#ifndef HAIKENANIME_SETTINGS_H
#define HAIKENANIME_SETTINGS_H

#include "../../domain/anilist/AniListSettings.h"
#include "../../domain/http/HttpSettings.h"

/** Groups application settings without exposing their file format to consumers. */
struct Settings {
    AniListSettings aniList;
    HttpSettings http;
    int syncTimeoutMs = 60000;
    int syncIntervalMs = 3600000;
    int logRetentionDays = 7;
};

#endif // HAIKENANIME_SETTINGS_H
