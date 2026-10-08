#include "SqliteSyncTaskStateRepository.h"

#include <QSqlError>
#include <QSqlQuery>

#include "SqlQueryStore.h"

#include <limits>
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

std::optional<int> ReadOptionalInt(const QVariant &value, bool &valid) {
    if (value.isNull()) return std::nullopt;
    bool converted = false;
    const auto number = value.toLongLong(&converted);
    if (!converted || number < std::numeric_limits<int>::min() || number > std::numeric_limits<int>::max()) {
        valid = false;
        return std::nullopt;
    }
    return static_cast<int>(number);
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
        const auto status = SyncTaskStatusFromString(query.value(2).toString());
        const auto cacheValidity = CacheValidityFromString(query.value(3).toString());
        const auto errorCategory = ErrorCategoryFromString(query.value(7).toString());
        bool timestampsValid = true;
        const auto lastSucceededAt = ReadTimestamp(query.value(4), timestampsValid);
        const auto lastAttemptedAt = ReadTimestamp(query.value(5), timestampsValid);
        const auto nextRunAt = ReadTimestamp(query.value(6), timestampsValid);
        const auto confirmedPage = ReadOptionalInt(query.value(9), timestampsValid);
        bool generationValid = false;
        const auto generation = query.value(12).toLongLong(&generationValid);
        const auto consecutiveFailures = query.value(13).toInt();
        const auto consecutiveImmediateRetries = query.value(14).toInt();
        if (!kind.has_value() || !partition.has_value() || !status.has_value() || !cacheValidity.has_value()
            || !errorCategory.has_value() || !timestampsValid || query.value(8).isNull() || !generationValid
            || generation < 0 || consecutiveFailures < 0 || consecutiveImmediateRetries < 0) {
            continue;
        }
        SyncTaskState state;
        state.kind = *kind;
        state.partition = *partition;
        state.status = *status;
        state.cacheValidity = *cacheValidity;
        state.lastSucceededAt = lastSucceededAt;
        state.lastAttemptedAt = lastAttemptedAt;
        state.nextRunAt = nextRunAt;
        state.lastErrorCategory = *errorCategory;
        state.safeErrorDetail = query.value(8).toString();
        state.confirmedPage = confirmedPage;
        state.confirmedCursor = query.value(10).isNull() ? std::nullopt
                                                          : std::optional<QString>(query.value(10).toString());
        state.priority = query.value(11).toInt();
        state.generation = generation;
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
    const auto status = ToString(state.status);
    const auto cacheValidity = ToString(state.cacheValidity);
    const auto errorCategory = ErrorCategoryToString(state.lastErrorCategory);
    if (kind.isEmpty() || partition.isEmpty() || status.isEmpty() || cacheValidity.isEmpty() || errorCategory.isEmpty()
        || state.confirmedPage.has_value() && *state.confirmedPage < 0 || state.generation < 0
        || state.consecutiveFailures < 0 || state.consecutiveImmediateRetries < 0
        || (state.lastSucceededAt.has_value() && !state.lastSucceededAt->isValid())
        || (state.lastAttemptedAt.has_value() && !state.lastAttemptedAt->isValid())
        || (state.nextRunAt.has_value() && !state.nextRunAt->isValid())) {
        error = QStringLiteral("Sync task state requires valid persisted values.");
        return false;
    }
    QString sql;
    QSqlQuery query(database_);
    if (!SqlQueryStore::loadSource(upsertQuery_, sql, error)) return false;
    if (!query.prepare(sql)) {
        error = query.lastError().text();
        return false;
    }
    query.bindValue(QStringLiteral(":task_kind"), kind);
    query.bindValue(QStringLiteral(":partition"), partition);
    query.bindValue(QStringLiteral(":status"), status);
    query.bindValue(QStringLiteral(":cache_validity"), cacheValidity);
    query.bindValue(QStringLiteral(":last_succeeded_at"), state.lastSucceededAt.has_value()
        ? state.lastSucceededAt->toUTC().toString(Qt::ISODate) : QVariant{});
    query.bindValue(QStringLiteral(":last_attempted_at"), state.lastAttemptedAt.has_value()
        ? state.lastAttemptedAt->toUTC().toString(Qt::ISODate) : QVariant{});
    query.bindValue(QStringLiteral(":next_run_at"), state.nextRunAt.has_value()
        ? state.nextRunAt->toUTC().toString(Qt::ISODate) : QVariant{});
    query.bindValue(QStringLiteral(":last_error_category"), errorCategory);
    query.bindValue(QStringLiteral(":safe_error_detail"), state.safeErrorDetail.isNull()
        ? QStringLiteral("") : state.safeErrorDetail);
    query.bindValue(QStringLiteral(":confirmed_page"), state.confirmedPage.has_value() ? *state.confirmedPage : QVariant{});
    query.bindValue(QStringLiteral(":confirmed_cursor"), state.confirmedCursor.has_value() ? *state.confirmedCursor : QVariant{});
    query.bindValue(QStringLiteral(":priority"), state.priority);
    query.bindValue(QStringLiteral(":generation"), state.generation);
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
    if (!SqlQueryStore::loadSource(deleteQuery_, sql, error)) return false;
    if (!query.prepare(sql)) {
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
