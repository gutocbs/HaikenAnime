#ifndef HAIKENANIME_SETTINGS_H
#define HAIKENANIME_SETTINGS_H

#include "AniListSettings.h"
#include "HttpSettings.h"
#include "UserPreferences.h"
#include "../covers/CoverSettings.h"

/** Groups application settings without exposing their file format to consumers. */
struct Settings {
    AniListSettings aniList;
    HttpSettings http;
    CoverSettings covers;
    UserPreferences userPreferences;
    int syncTimeoutMs = 60000;
    int syncIntervalMs = 3600000;
    int logRetentionDays = 7;
};

#endif // HAIKENANIME_SETTINGS_H
