#ifndef HAIKENANIME_SYNCTASKTYPES_H
#define HAIKENANIME_SYNCTASKTYPES_H

#include <QDateTime>
#include <QString>

#include <algorithm>
#include <chrono>
#include <map>
#include <optional>

#include "../anilist/AniListSyncErrorCategory.h"

// Kind and partition are 1:1 but persisted in separate columns with different texts
// ("pending-change" x "pending-changes"). Rows of any other text are skipped when read.
enum class SyncTaskKind { UserList, PendingChange };
enum class SyncPartition { UserList, PendingChanges };
enum class CacheValidity { Unknown, Fresh, Stale, Expired };
enum class SyncTaskStatus { Idle, Running, RetryScheduled, Failed, Succeeded };

inline QString ToString(SyncTaskKind value) {
    switch (value) {
    case SyncTaskKind::UserList: return QStringLiteral("user-list");
    case SyncTaskKind::PendingChange: return QStringLiteral("pending-change");
    }
    return {};
}

inline std::optional<SyncTaskKind> SyncTaskKindFromString(const QString &value) {
    for (const auto kind : {SyncTaskKind::UserList, SyncTaskKind::PendingChange}) {
        if (ToString(kind) == value) return kind;
    }
    return std::nullopt;
}

inline QString ToString(SyncPartition value) {
    switch (value) {
    case SyncPartition::UserList: return QStringLiteral("user-list");
    case SyncPartition::PendingChanges: return QStringLiteral("pending-changes");
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

inline QString ToString(SyncTaskStatus value) {
    switch (value) {
    case SyncTaskStatus::Idle: return QStringLiteral("idle");
    case SyncTaskStatus::Running: return QStringLiteral("running");
    case SyncTaskStatus::RetryScheduled: return QStringLiteral("retry-scheduled");
    case SyncTaskStatus::Failed: return QStringLiteral("failed");
    case SyncTaskStatus::Succeeded: return QStringLiteral("succeeded");
    }
    return {};
}

inline std::optional<SyncTaskStatus> SyncTaskStatusFromString(const QString &value) {
    for (const auto status : {SyncTaskStatus::Idle, SyncTaskStatus::Running, SyncTaskStatus::RetryScheduled,
                              SyncTaskStatus::Failed, SyncTaskStatus::Succeeded}) {
        if (ToString(status) == value) return status;
    }
    return std::nullopt;
}

struct SyncTaskState final {
    SyncTaskKind kind = SyncTaskKind::UserList;
    SyncPartition partition = SyncPartition::UserList;
    SyncTaskStatus status = SyncTaskStatus::Idle;
    CacheValidity cacheValidity = CacheValidity::Unknown;
    std::optional<QDateTime> lastSucceededAt;
    std::optional<QDateTime> lastAttemptedAt;
    std::optional<QDateTime> nextRunAt;
    AniListSyncErrorCategory lastErrorCategory = AniListSyncErrorCategory::None;
    QString safeErrorDetail;
    std::optional<int> confirmedPage;
    int priority = 0;
    qint64 generation = 0;
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
    }
    return policy;
}

/// Policy with another normal interval; the stale-data TTL grows with it so it never ends before the next run.
inline SyncSchedulePolicy WithNormalInterval(SyncSchedulePolicy policy, const std::chrono::milliseconds interval) {
    policy.normalInterval = interval;
    policy.staleProtectionTtl = std::max(policy.staleProtectionTtl, interval);
    return policy;
}

inline std::map<SyncTaskKind, SyncSchedulePolicy> DefaultSyncTaskPolicies() {
    return {{SyncTaskKind::UserList, DefaultSyncSchedulePolicy(SyncTaskKind::UserList)},
            {SyncTaskKind::PendingChange, DefaultSyncSchedulePolicy(SyncTaskKind::PendingChange)}};
}

#endif // HAIKENANIME_SYNCTASKTYPES_H
