#include "SqlitePluginSettingsRepository.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlError>
#include <QSqlQuery>

#include "SqliteQueries.h"
#include "SqlQueryStore.h"

#include <utility>

namespace {
// Values are wrapped in a one-element JSON array so scalars keep their JSON type (bool, number, string).
QString EncodeValue(const QVariant &value) {
    return QString::fromUtf8(QJsonDocument(QJsonArray{QJsonValue::fromVariant(value)}).toJson(QJsonDocument::Compact));
}

bool DecodeValue(const QString &text, QVariant &value) {
    const auto document = QJsonDocument::fromJson(text.toUtf8());
    if (!document.isArray() || document.array().size() != 1) return false;
    value = document.array().first().toVariant();
    return true;
}
}

SqlitePluginSettingsRepository::SqlitePluginSettingsRepository(QSqlDatabase database)
    : database_(std::move(database)) {}

bool SqlitePluginSettingsRepository::read(const QString &pluginId, QVariantMap &values, QString &error) {
    values.clear();
    error.clear();
    QString sql;
    if (!SqlQueryStore::loadSource(SqliteQueries::ReadPluginSettings, sql, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(sql)) {
        error = query.lastError().text();
        return false;
    }
    query.bindValue(QStringLiteral(":plugin_id"), pluginId);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    QVariantMap loaded;
    while (query.next()) {
        const auto key = query.value(0).toString();
        QVariant value;
        if (!DecodeValue(query.value(1).toString(), value)) {
            error = QStringLiteral("Stored plugin setting '%1' of '%2' is invalid.").arg(key, pluginId);
            return false;
        }
        loaded.insert(key, value);
    }
    values = std::move(loaded);
    return true;
}

bool SqlitePluginSettingsRepository::replace(const QString &pluginId, const QVariantMap &values, QString &error) {
    error.clear();
    QString deleteSql;
    QString upsertSql;
    if (!SqlQueryStore::loadSource(SqliteQueries::DeletePluginSettings, deleteSql, error)
        || !SqlQueryStore::loadSource(SqliteQueries::UpsertPluginSettings, upsertSql, error)) {
        return false;
    }
    if (!database_.transaction()) {
        error = database_.lastError().text();
        return false;
    }
    const auto fail = [this, &error](const QSqlQuery &query) {
        error = query.lastError().text();
        database_.rollback();
        return false;
    };

    QSqlQuery remove(database_);
    if (!remove.prepare(deleteSql)) return fail(remove);
    remove.bindValue(QStringLiteral(":plugin_id"), pluginId);
    if (!remove.exec()) return fail(remove);

    QSqlQuery upsert(database_);
    if (!upsert.prepare(upsertSql)) return fail(upsert);
    for (auto entry = values.cbegin(); entry != values.cend(); ++entry) {
        upsert.bindValue(QStringLiteral(":plugin_id"), pluginId);
        upsert.bindValue(QStringLiteral(":setting_key"), entry.key());
        upsert.bindValue(QStringLiteral(":setting_value"), EncodeValue(entry.value()));
        if (!upsert.exec()) return fail(upsert);
    }

    if (!database_.commit()) {
        error = database_.lastError().text();
        database_.rollback();
        return false;
    }
    return true;
}
