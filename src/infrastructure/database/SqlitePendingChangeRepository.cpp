#include "SqlitePendingChangeRepository.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>

#include <utility>

namespace {
QString EncodeValue(const AniListFieldValue &value) {
    QJsonObject object;
    if (std::holds_alternative<int>(value)) {
        object.insert(QStringLiteral("type"), QStringLiteral("int"));
        object.insert(QStringLiteral("value"), std::get<int>(value));
    } else {
        object.insert(QStringLiteral("type"), QStringLiteral("string"));
        object.insert(QStringLiteral("value"), std::get<QString>(value));
    }
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

AniListFieldValue DecodeValue(const QString &encoded) {
    const auto object = QJsonDocument::fromJson(encoded.toUtf8()).object();
    if (object.value(QStringLiteral("type")).toString() == QStringLiteral("int")) {
        return object.value(QStringLiteral("value")).toInt();
    }
    return object.value(QStringLiteral("value")).toString();
}
}

SqlitePendingChangeRepository::SqlitePendingChangeRepository(
    QSqlDatabase database, QString enqueueQuery, QString pendingQuery, QString updateStatusQuery)
    : database_(std::move(database)), enqueueQuery_(std::move(enqueueQuery)),
      pendingQuery_(std::move(pendingQuery)), updateStatusQuery_(std::move(updateStatusQuery)) {
}

bool SqlitePendingChangeRepository::enqueue(const AniListPendingChange &change, QString &error) {
    error.clear();
    QSqlQuery query(database_);
    query.prepare(enqueueQuery_);
    query.bindValue(QStringLiteral(":media_id"), change.mediaId);
    query.bindValue(QStringLiteral(":field"), static_cast<int>(change.field));
    query.bindValue(QStringLiteral(":previous_value"), EncodeValue(change.previousValue));
    query.bindValue(QStringLiteral(":new_value"), EncodeValue(change.newValue));
    query.bindValue(QStringLiteral(":created_at"), change.createdAt.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":local_updated_at"), change.localUpdatedAt.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":remote_observed_at"), change.remoteObservedAt.isValid()
                                                             ? change.remoteObservedAt.toString(Qt::ISODate)
                                                             : QVariant());
    query.bindValue(QStringLiteral(":remote_version"), change.remoteVersion);
    query.bindValue(QStringLiteral(":attempts"), change.attempts);
    query.bindValue(QStringLiteral(":status"), static_cast<int>(change.status));
    query.bindValue(QStringLiteral(":last_error"), change.lastError);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    return true;
}

bool SqlitePendingChangeRepository::getPending(int mediaId, QList<AniListPendingChange> &changes,
                                               QString &error) {
    changes.clear();
    error.clear();
    QSqlQuery query(database_);
    query.prepare(pendingQuery_);
    query.bindValue(QStringLiteral(":media_id"), mediaId);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    while (query.next()) {
        AniListPendingChange change;
        change.id = query.value(QStringLiteral("id")).toLongLong();
        change.mediaId = query.value(QStringLiteral("media_id")).toInt();
        change.field = static_cast<AniListField>(query.value(QStringLiteral("field")).toInt());
        change.previousValue = DecodeValue(query.value(QStringLiteral("previous_value")).toString());
        change.newValue = DecodeValue(query.value(QStringLiteral("new_value")).toString());
        change.createdAt = QDateTime::fromString(query.value(QStringLiteral("created_at")).toString(), Qt::ISODate);
        change.localUpdatedAt = QDateTime::fromString(query.value(QStringLiteral("local_updated_at")).toString(), Qt::ISODate);
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
    QSqlQuery query(database_);
    query.prepare(updateStatusQuery_);
    query.bindValue(QStringLiteral(":id"), change.id);
    query.bindValue(QStringLiteral(":status"), static_cast<int>(change.status));
    query.bindValue(QStringLiteral(":attempts"), change.attempts);
    query.bindValue(QStringLiteral(":last_error"), change.lastError);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    return true;
}
