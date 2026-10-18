#include "SqlitePendingChangeRepository.h"

#include <QSqlError>
#include <QSqlQuery>

#include <utility>
#include "SqlQueryStore.h"
#include "SqlitePendingChangeCodec.h"
#include "SqliteQueries.h"

SqlitePendingChangeRepository::SqlitePendingChangeRepository(QSqlDatabase database)
    : database_(std::move(database)) {
}

bool SqlitePendingChangeRepository::getPendingMediaIds(QList<int> &mediaIds, QString &error) {
    mediaIds.clear();
    error.clear();
    QString source;
    if (!SqlQueryStore::loadSource(SqliteQueries::ReadPendingMediaIds, source, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(source)) {
        error = query.lastError().text();
        return false;
    }
    query.bindValue(QStringLiteral(":owner"), ownerUserId_);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    while (query.next()) mediaIds.append(query.value(QStringLiteral("media_id")).toInt());
    return true;
}

bool SqlitePendingChangeRepository::getMediaIdsWithUnsentChanges(QSet<int> &mediaIds, QString &error) {
    mediaIds.clear();
    error.clear();
    QString source;
    if (!SqlQueryStore::loadSource(SqliteQueries::ReadUnsentMediaIds, source, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(source)) {
        error = query.lastError().text();
        return false;
    }
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    while (query.next()) mediaIds.insert(query.value(QStringLiteral("media_id")).toInt());
    return true;
}

bool SqlitePendingChangeRepository::enqueue(const AniListPendingChange &change, QString &error) {
    error.clear();
    QString enqueueQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::EnqueuePendingChange, enqueueQuery, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(enqueueQuery)) { error = query.lastError().text(); return false; }
    SqlitePendingChangeCodec::BindEnqueue(query, change);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    return true;
}

void SqlitePendingChangeRepository::setOwnerUserId(const qint64 ownerUserId) {
    ownerUserId_ = ownerUserId;
}

bool SqlitePendingChangeRepository::adoptOrphanChanges(const qint64 ownerUserId, int &adoptedCount,
                                                       QString &error) {
    adoptedCount = 0;
    error.clear();
    if (ownerUserId <= 0) {
        error = QStringLiteral("Orphan pending changes can only be adopted by an authenticated account.");
        return false;
    }
    QString source;
    if (!SqlQueryStore::loadSource(SqliteQueries::AdoptOrphanPendingChanges, source, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(source)) { error = query.lastError().text(); return false; }
    query.bindValue(QStringLiteral(":owner"), ownerUserId);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    adoptedCount = query.numRowsAffected();
    return true;
}

bool SqlitePendingChangeRepository::getPending(int mediaId, QList<AniListPendingChange> &changes,
                                               QString &error) {
    changes.clear();
    error.clear();
    QString pendingQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::ReadPendingChanges, pendingQuery, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(pendingQuery)) { error = query.lastError().text(); return false; }
    query.bindValue(QStringLiteral(":media_id"), mediaId);
    query.bindValue(QStringLiteral(":owner"), ownerUserId_);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    while (query.next()) {
        AniListPendingChange change;
        change.id = query.value(QStringLiteral("id")).toLongLong();
        change.ownerUserId = query.value(QStringLiteral("owner_user_id")).toLongLong();
        change.mediaId = query.value(QStringLiteral("media_id")).toInt();
        change.field = static_cast<AniListField>(query.value(QStringLiteral("field")).toInt());
        if (!SqlitePendingChangeCodec::DecodeValue(query.value(QStringLiteral("previous_value")).toString(),
                         change.previousValue)
            || !SqlitePendingChangeCodec::DecodeValue(query.value(QStringLiteral("new_value")).toString(),
                            change.newValue)) {
            changes.clear();
            error = QStringLiteral("Pending AniList change %1 contains an invalid encoded value.")
                        .arg(change.id);
            return false;
        }
        change.createdAt = QDateTime::fromString(query.value(QStringLiteral("created_at")).toString(), Qt::ISODate);
        change.localUpdatedAt = QDateTime::fromString(query.value(QStringLiteral("local_updated_at")).toString(), Qt::ISODate);
        change.remoteObservedAt = QDateTime::fromString(
            query.value(QStringLiteral("remote_observed_at")).toString(), Qt::ISODate);
        change.remoteVersion = query.value(QStringLiteral("remote_version")).toString();
        change.attempts = query.value(QStringLiteral("attempts")).toInt();
        change.status = static_cast<AniListPendingChangeStatus>(query.value(QStringLiteral("status")).toInt());
        change.lastError = query.value(QStringLiteral("last_error")).toString();
        changes.append(change);
    }
    return true;
}

bool SqlitePendingChangeRepository::updateStatus(const AniListPendingChange &change, QString &error) {
    error.clear();
    QString updateStatusQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::UpdatePendingChange, updateStatusQuery, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(updateStatusQuery)) { error = query.lastError().text(); return false; }
    query.bindValue(QStringLiteral(":id"), change.id);
    query.bindValue(QStringLiteral(":status"), static_cast<int>(change.status));
    query.bindValue(QStringLiteral(":attempts"), change.attempts);
    query.bindValue(QStringLiteral(":last_error"),
                    change.lastError.isNull() ? QStringLiteral("") : change.lastError);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() != 1) {
        error = QStringLiteral("Pending AniList change %1 was not updated.").arg(change.id);
        return false;
    }
    return true;
}
