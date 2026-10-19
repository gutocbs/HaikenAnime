#include "SqlitePendingChangeCodec.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>

#include <cmath>
#include <limits>

namespace SqlitePendingChangeCodec {
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

void BindEnqueue(QSqlQuery &query, const AniListPendingChange &change) {
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
    query.bindValue(QStringLiteral(":last_error"), change.lastError.isNull() ? QStringLiteral("") : change.lastError);
    query.bindValue(QStringLiteral(":owner_user_id"), change.ownerUserId);
}
}
