#ifndef HAIKENANIME_SETTINGS_H
#define HAIKENANIME_SETTINGS_H

#include "AniListSettings.h"
#include "HttpSettings.h"
#include "UserPreferences.h"
#include "../covers/CoverSettings.h"
#include "../catalog/SeasonalCatalogTypes.h"
#include "../scheduling/SyncTaskTypes.h"

#include <map>

/** Groups application settings without exposing their file format to consumers. */
struct Settings {
    AniListSettings aniList;
    HttpSettings http;
    CoverSettings covers;
    UserPreferences userPreferences;
    SeasonalCatalogCachePolicy seasonalCatalogCachePolicy;
    int syncTimeoutMs = 60000;
    int syncIntervalMs = 3600000;
    std::map<SyncTaskKind, SyncSchedulePolicy> syncTaskPolicies{
        {SyncTaskKind::UserList, {}}, {SyncTaskKind::PendingChange, {}},
        {SyncTaskKind::ActiveCatalog, {}}, {SyncTaskKind::InactiveCatalog, {}},
        {SyncTaskKind::CompletedCatalog, {}}, {SyncTaskKind::Cover, {}},
        {SyncTaskKind::DerivedMetadata, {}}};
    int logRetentionDays = 7;
};

#endif // HAIKENANIME_SETTINGS_H
