#include "SqliteSyncTaskStateRepository.h"

#include <QSqlError>
#include <QSqlQuery>

#include "SqlQueryStore.h"

#include <utility>

namespace {
std::optional<SyncPartition> PartitionFromString(const QString &value) {
    for (const auto partition : {SyncPartition::UserList, SyncPartition::PendingChanges, SyncPartition::ActiveCatalog,
                                 SyncPartition::InactiveCatalog, SyncPartition::CompletedCatalog, SyncPartition::Covers,
                                 SyncPartition::DerivedMetadata}) {
        if (ToString(partition) == value) return partition;
    }
    return std::nullopt;
}

std::optional<CacheValidity> CacheValidityFromString(const QString &value) {
    for (const auto validity : {CacheValidity::Unknown, CacheValidity::Fresh, CacheValidity::Stale, CacheValidity::Expired}) {
        if (ToString(validity) == value) return validity;
    }
    return std::nullopt;
}

QString ErrorCategoryToString(const AniListSyncErrorCategory value) {
    switch (value) {
    case AniListSyncErrorCategory::None: return QStringLiteral("none");
    case AniListSyncErrorCategory::Authentication: return QStringLiteral("authentication");
    case AniListSyncErrorCategory::Authorization: return QStringLiteral("authorization");
    case AniListSyncErrorCategory::RateLimit: return QStringLiteral("rate-limit");
    case AniListSyncErrorCategory::GraphQl: return QStringLiteral("graphql");
    case AniListSyncErrorCategory::Network: return QStringLiteral("network");
    case AniListSyncErrorCategory::Timeout: return QStringLiteral("timeout");
    case AniListSyncErrorCategory::Persistence: return QStringLiteral("persistence");
    case AniListSyncErrorCategory::InvalidData: return QStringLiteral("invalid-data");
    case AniListSyncErrorCategory::Cancelled: return QStringLiteral("cancelled");
    case AniListSyncErrorCategory::Unknown: return QStringLiteral("unknown");
    }
    return {};
}

std::optional<AniListSyncErrorCategory> ErrorCategoryFromString(const QString &value) {
    for (const auto category : {AniListSyncErrorCategory::None, AniListSyncErrorCategory::Authentication,
                                AniListSyncErrorCategory::Authorization, AniListSyncErrorCategory::RateLimit,
                                AniListSyncErrorCategory::GraphQl, AniListSyncErrorCategory::Network,
                                AniListSyncErrorCategory::Timeout, AniListSyncErrorCategory::Persistence,
                                AniListSyncErrorCategory::InvalidData, AniListSyncErrorCategory::Cancelled,
                                AniListSyncErrorCategory::Unknown}) {
        if (ErrorCategoryToString(category) == value) return category;
    }
    return std::nullopt;
}

std::optional<QDateTime> ReadTimestamp(const QVariant &value, bool &valid) {
    if (value.isNull()) return std::nullopt;
    const auto timestamp = QDateTime::fromString(value.toString(), Qt::ISODate);
    if (!timestamp.isValid()) {
        valid = false;
        return std::nullopt;
    }
    return timestamp.toUTC();
}
}

SqliteSyncTaskStateRepository::SqliteSyncTaskStateRepository(QSqlDatabase database, QString readQuery,
                                                             QString upsertQuery, QString deleteQuery)
    : database_(std::move(database)), readQuery_(std::move(readQuery)), upsertQuery_(std::move(upsertQuery)),
      deleteQuery_(std::move(deleteQuery)) {
}

bool SqliteSyncTaskStateRepository::ReadAll(QList<SyncTaskState> &states, QString &error) {
    states.clear();
    error.clear();
    QString sql;
    if (!SqlQueryStore::loadSource(readQuery_, sql, error)) return false;
    QSqlQuery query(database_);
    if (!query.exec(sql)) {
        error = query.lastError().text();
        return false;
    }
    while (query.next()) {
        const auto kind = SyncTaskKindFromString(query.value(0).toString());
        const auto partition = PartitionFromString(query.value(1).toString());
        const auto cacheValidity = CacheValidityFromString(query.value(2).toString());
        const auto errorCategory = ErrorCategoryFromString(query.value(5).toString());
        bool timestampsValid = true;
        const auto lastSucceededAt = ReadTimestamp(query.value(3), timestampsValid);
        const auto lastAttemptedAt = ReadTimestamp(query.value(4), timestampsValid);
        const auto consecutiveFailures = query.value(6).toInt();
        const auto consecutiveImmediateRetries = query.value(7).toInt();
        if (!kind.has_value() || !partition.has_value() || !cacheValidity.has_value() || !errorCategory.has_value()
            || !timestampsValid || consecutiveFailures < 0 || consecutiveImmediateRetries < 0) {
            continue;
        }
        SyncTaskState state;
        state.kind = *kind;
        state.partition = *partition;
        state.cacheValidity = *cacheValidity;
        state.lastSucceededAt = lastSucceededAt;
        state.lastAttemptedAt = lastAttemptedAt;
        state.lastErrorCategory = *errorCategory;
        state.consecutiveFailures = consecutiveFailures;
        state.consecutiveImmediateRetries = consecutiveImmediateRetries;
        states.append(state);
    }
    return true;
}

bool SqliteSyncTaskStateRepository::Upsert(const SyncTaskState &state, QString &error) {
    error.clear();
    const auto kind = ToString(state.kind);
    const auto partition = ToString(state.partition);
    const auto cacheValidity = ToString(state.cacheValidity);
    const auto errorCategory = ErrorCategoryToString(state.lastErrorCategory);
    if (kind.isEmpty() || partition.isEmpty() || cacheValidity.isEmpty() || errorCategory.isEmpty()
        || state.consecutiveFailures < 0 || state.consecutiveImmediateRetries < 0
        || (state.lastSucceededAt.has_value() && !state.lastSucceededAt->isValid())
        || (state.lastAttemptedAt.has_value() && !state.lastAttemptedAt->isValid())) {
        error = QStringLiteral("Sync task state requires valid persisted values.");
        return false;
    }
    QString sql;
    QSqlQuery query(database_);
    if (!SqlQueryStore::loadSource(upsertQuery_, sql, error) || !query.prepare(sql)) {
        error = query.lastError().text();
        return false;
    }
    query.bindValue(QStringLiteral(":task_kind"), kind);
    query.bindValue(QStringLiteral(":partition"), partition);
    query.bindValue(QStringLiteral(":cache_validity"), cacheValidity);
    query.bindValue(QStringLiteral(":last_succeeded_at"), state.lastSucceededAt.has_value()
        ? state.lastSucceededAt->toUTC().toString(Qt::ISODate) : QVariant{});
    query.bindValue(QStringLiteral(":last_attempted_at"), state.lastAttemptedAt.has_value()
        ? state.lastAttemptedAt->toUTC().toString(Qt::ISODate) : QVariant{});
    query.bindValue(QStringLiteral(":last_error_category"), errorCategory);
    query.bindValue(QStringLiteral(":consecutive_failures"), state.consecutiveFailures);
    query.bindValue(QStringLiteral(":consecutive_immediate_retries"), state.consecutiveImmediateRetries);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    return true;
}

bool SqliteSyncTaskStateRepository::Remove(const SyncPartition &partition, QString &error) {
    error.clear();
    const auto partitionKey = ToString(partition);
    if (partitionKey.isEmpty()) {
        error = QStringLiteral("Sync task partition is invalid.");
        return false;
    }
    QString sql;
    QSqlQuery query(database_);
    if (!SqlQueryStore::loadSource(deleteQuery_, sql, error) || !query.prepare(sql)) {
        error = query.lastError().text();
        return false;
    }
    query.bindValue(QStringLiteral(":partition"), partitionKey);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    return true;
}
