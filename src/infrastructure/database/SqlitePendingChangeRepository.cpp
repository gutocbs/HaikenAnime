#include "SqlitePendingChangeRepository.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>

#include <cmath>
#include <limits>
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

bool DecodeValue(const QString &encoded, AniListFieldValue &value) {
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(encoded.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }
    const auto object = document.object();
    const auto type = object.value(QStringLiteral("type")).toString();
    const auto encodedValue = object.value(QStringLiteral("value"));
    if (type == QStringLiteral("int") && encodedValue.isDouble()) {
        const double number = encodedValue.toDouble();
        if (!std::isfinite(number) || std::trunc(number) != number
            || number < std::numeric_limits<int>::min()
            || number > std::numeric_limits<int>::max()) {
            return false;
        }
        value = static_cast<int>(number);
        return true;
    }
    if (type == QStringLiteral("string") && encodedValue.isString()) {
        value = encodedValue.toString();
        return true;
    }
    return false;
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
    query.bindValue(QStringLiteral(":remote_version"),
                    change.remoteVersion.isNull() ? QStringLiteral("") : change.remoteVersion);
    query.bindValue(QStringLiteral(":attempts"), change.attempts);
    query.bindValue(QStringLiteral(":status"), static_cast<int>(change.status));
    query.bindValue(QStringLiteral(":last_error"),
                    change.lastError.isNull() ? QStringLiteral("") : change.lastError);
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
        if (!DecodeValue(query.value(QStringLiteral("previous_value")).toString(),
                         change.previousValue)
            || !DecodeValue(query.value(QStringLiteral("new_value")).toString(),
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
    if (query.numRowsAffected() != 1) {
        error = QStringLiteral("Pending AniList change %1 was not updated.").arg(change.id);
        return false;
    }
    return true;
}
