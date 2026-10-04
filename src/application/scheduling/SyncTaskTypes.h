#ifndef HAIKENANIME_SYNCTASKTYPES_H
#define HAIKENANIME_SYNCTASKTYPES_H

#include <QDateTime>
#include <QString>

#include <chrono>
#include <map>
#include <optional>

#include "../anilist/AniListSyncErrorCategory.h"

enum class SyncTaskKind { UserList, PendingChange, ActiveCatalog, InactiveCatalog, CompletedCatalog, Cover, DerivedMetadata };
enum class SyncPartition { UserList, PendingChanges, ActiveCatalog, InactiveCatalog, CompletedCatalog, Covers, DerivedMetadata };
enum class CacheValidity { Unknown, Fresh, Stale, Expired };

inline QString ToString(SyncTaskKind value) {
    switch (value) {
    case SyncTaskKind::UserList: return QStringLiteral("user-list");
    case SyncTaskKind::PendingChange: return QStringLiteral("pending-change");
    case SyncTaskKind::ActiveCatalog: return QStringLiteral("active-catalog");
    case SyncTaskKind::InactiveCatalog: return QStringLiteral("inactive-catalog");
    case SyncTaskKind::CompletedCatalog: return QStringLiteral("completed-catalog");
    case SyncTaskKind::Cover: return QStringLiteral("cover");
    case SyncTaskKind::DerivedMetadata: return QStringLiteral("derived-metadata");
    }
    return {};
}

inline std::optional<SyncTaskKind> SyncTaskKindFromString(const QString &value) {
    for (const auto kind : {SyncTaskKind::UserList, SyncTaskKind::PendingChange, SyncTaskKind::ActiveCatalog,
                            SyncTaskKind::InactiveCatalog, SyncTaskKind::CompletedCatalog, SyncTaskKind::Cover,
                            SyncTaskKind::DerivedMetadata}) {
        if (ToString(kind) == value) return kind;
    }
    return std::nullopt;
}

inline QString ToString(SyncPartition value) {
    switch (value) {
    case SyncPartition::UserList: return QStringLiteral("user-list");
    case SyncPartition::PendingChanges: return QStringLiteral("pending-changes");
    case SyncPartition::ActiveCatalog: return QStringLiteral("active-catalog");
    case SyncPartition::InactiveCatalog: return QStringLiteral("inactive-catalog");
    case SyncPartition::CompletedCatalog: return QStringLiteral("completed-catalog");
    case SyncPartition::Covers: return QStringLiteral("covers");
    case SyncPartition::DerivedMetadata: return QStringLiteral("derived-metadata");
    }
    return {};
}

inline QString ToString(CacheValidity value) {
    switch (value) {
    case CacheValidity::Unknown: return QStringLiteral("unknown");
    case CacheValidity::Fresh: return QStringLiteral("fresh");
    case CacheValidity::Stale: return QStringLiteral("stale");
    case CacheValidity::Expired: return QStringLiteral("expired");
    }
    return {};
}

struct SyncTaskState final {
    SyncTaskKind kind = SyncTaskKind::UserList;
    SyncPartition partition = SyncPartition::UserList;
    CacheValidity cacheValidity = CacheValidity::Unknown;
    std::optional<QDateTime> lastSucceededAt;
    std::optional<QDateTime> lastAttemptedAt;
    AniListSyncErrorCategory lastErrorCategory = AniListSyncErrorCategory::None;
    int consecutiveFailures = 0;
    int consecutiveImmediateRetries = 0;
};

struct SyncSchedulePolicy final {
    std::chrono::milliseconds normalInterval = std::chrono::hours(1);
    std::chrono::milliseconds staleProtectionTtl = std::chrono::hours(24);
    std::chrono::milliseconds initialRetryDelay = std::chrono::seconds(1);
    std::chrono::milliseconds maximumRetryDelay = std::chrono::minutes(5);
    std::chrono::milliseconds cooldown = std::chrono::hours(1);
    double jitterRatio = 0.0;
    int maximumConsecutiveImmediateRetries = 0;
};

struct SyncScheduleDecision final {
    bool shouldRunNow = false;
    QDateTime nextRunAt;
    CacheValidity cacheValidity = CacheValidity::Unknown;
};

inline SyncSchedulePolicy DefaultSyncSchedulePolicy(SyncTaskKind kind) {
    SyncSchedulePolicy policy;
    switch (kind) {
    case SyncTaskKind::UserList:
        policy.normalInterval = std::chrono::hours(1);
        policy.staleProtectionTtl = std::chrono::hours(24);
        break;
    case SyncTaskKind::PendingChange:
        policy.normalInterval = std::chrono::minutes(1);
        policy.staleProtectionTtl = std::chrono::hours(1);
        break;
    case SyncTaskKind::ActiveCatalog:
        policy.normalInterval = std::chrono::minutes(30);
        policy.staleProtectionTtl = std::chrono::hours(6);
        break;
    case SyncTaskKind::InactiveCatalog:
        policy.normalInterval = std::chrono::hours(12);
        policy.staleProtectionTtl = std::chrono::hours(24 * 7);
        break;
    case SyncTaskKind::CompletedCatalog:
        policy.normalInterval = std::chrono::hours(24);
        policy.staleProtectionTtl = std::chrono::hours(24 * 14);
        break;
    case SyncTaskKind::Cover:
        policy.normalInterval = std::chrono::hours(6);
        policy.staleProtectionTtl = std::chrono::hours(48);
        break;
    case SyncTaskKind::DerivedMetadata:
        policy.normalInterval = std::chrono::hours(2);
        policy.staleProtectionTtl = std::chrono::hours(12);
        break;
    }
    return policy;
}

inline std::map<SyncTaskKind, SyncSchedulePolicy> DefaultSyncTaskPolicies() {
    return {{SyncTaskKind::UserList, DefaultSyncSchedulePolicy(SyncTaskKind::UserList)},
            {SyncTaskKind::PendingChange, DefaultSyncSchedulePolicy(SyncTaskKind::PendingChange)},
            {SyncTaskKind::ActiveCatalog, DefaultSyncSchedulePolicy(SyncTaskKind::ActiveCatalog)},
            {SyncTaskKind::InactiveCatalog, DefaultSyncSchedulePolicy(SyncTaskKind::InactiveCatalog)},
            {SyncTaskKind::CompletedCatalog, DefaultSyncSchedulePolicy(SyncTaskKind::CompletedCatalog)},
            {SyncTaskKind::Cover, DefaultSyncSchedulePolicy(SyncTaskKind::Cover)},
            {SyncTaskKind::DerivedMetadata, DefaultSyncSchedulePolicy(SyncTaskKind::DerivedMetadata)}};
}

#endif // HAIKENANIME_SYNCTASKTYPES_H
