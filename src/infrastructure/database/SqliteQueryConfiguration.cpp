#include "SqliteQueryConfiguration.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include "../logging/AsyncLogger.h"

void SqliteQueryConfiguration::setLogger(AsyncLogger *value) { logger = value; }

bool SqliteQueryConfiguration::load(QString &error) {
    const auto configurationPath = QStringLiteral(":/sqlite/queries/sqlite-queries.json");
    QFile file(configurationPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("Could not open SQLite query configuration '%1': %2")
                    .arg(configurationPath, file.errorString());
        if (logger) logger->error(LogCategory::QueryConfiguration, error);
        return false;
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    const auto queries = document.object().value(QStringLiteral("queries")).toObject();
    if (parseError.error != QJsonParseError::NoError || queries.isEmpty()) {
        error = QStringLiteral("Invalid SQLite query configuration '%1': %2")
                    .arg(configurationPath, parseError.errorString());
        if (logger) logger->error(LogCategory::QueryConfiguration, error);
        return false;
    }

    upsertMediaPath = queries.value(QStringLiteral("upsertMedia")).toString();
    readMediaPath = queries.value(QStringLiteral("readMedia")).toString();
    enqueuePendingChangePath = queries.value(QStringLiteral("enqueuePendingChange")).toString();
    readPendingChangesPath = queries.value(QStringLiteral("readPendingChanges")).toString();
    updatePendingChangePath = queries.value(QStringLiteral("updatePendingChange")).toString();
    if (upsertMediaPath.isEmpty() || readMediaPath.isEmpty() || enqueuePendingChangePath.isEmpty()
        || readPendingChangesPath.isEmpty() || updatePendingChangePath.isEmpty()) {
        error = QStringLiteral("SQLite query configuration is incomplete.");
        if (logger) logger->error(LogCategory::QueryConfiguration, error);
        return false;
    }
    if (logger) logger->info(LogCategory::QueryConfiguration, QStringLiteral("SQLite query configuration loaded."));
    return true;
}
