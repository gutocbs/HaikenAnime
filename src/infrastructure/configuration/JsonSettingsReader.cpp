#include "JsonSettingsReader.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include <cmath>
#include <limits>
#include <utility>

namespace {
bool readInteger(const QJsonObject &object, const QString &key, const int defaultValue,
                 int &result) {
    const auto value = object.value(key);
    if (value.isUndefined()) {
        result = defaultValue;
        return true;
    }
    if (!value.isDouble()) {
        return false;
    }
    const double number = value.toDouble();
    if (std::trunc(number) != number || number < std::numeric_limits<int>::min()
        || number > std::numeric_limits<int>::max()) {
        return false;
    }
    result = static_cast<int>(number);
    return true;
}
}

JsonSettingsReader::JsonSettingsReader(QString filePath)
    : filePath_(std::move(filePath)) {
}

bool JsonSettingsReader::read(Settings &settings, QString &error) {
    settings = {};
    error.clear();
    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("Could not open Settings.json: %1").arg(file.errorString());
        return false;
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("Invalid Settings.json: %1").arg(parseError.errorString());
        return false;
    }

    const auto root = document.object();
    const auto aniList = root.value(QStringLiteral("anilist")).toObject();
    const auto http = root.value(QStringLiteral("http")).toObject();
    if (aniList.isEmpty() || http.isEmpty()) {
        error = QStringLiteral("Settings.json must contain anilist and http objects.");
        return false;
    }

    settings.aniList.endpoint = aniList.value(QStringLiteral("endpoint")).toString();
    settings.aniList.mediaQueryFile = aniList.value(QStringLiteral("mediaQueryFile")).toString();
    const auto sync = root.value(QStringLiteral("sync")).toObject();
    const auto logging = root.value(QStringLiteral("logging")).toObject();
    if (!readInteger(http, QStringLiteral("timeoutMs"), 30000, settings.http.timeoutMs)
        || !readInteger(http, QStringLiteral("maxRetries"), 2, settings.http.maxRetries)
        || !readInteger(http, QStringLiteral("retryDelayMs"), 1000,
                        settings.http.retryDelayMs)
        || !readInteger(sync, QStringLiteral("timeoutMs"), 60000, settings.syncTimeoutMs)
        || !readInteger(sync, QStringLiteral("intervalMs"), 3600000,
                        settings.syncIntervalMs)
        || !readInteger(logging, QStringLiteral("retentionDays"), 7,
                        settings.logRetentionDays)) {
        error = QStringLiteral("Settings.json numeric settings must contain whole numbers.");
        return false;
    }

    if (settings.aniList.endpoint.isEmpty() || settings.aniList.mediaQueryFile.isEmpty()
        || settings.http.timeoutMs <= 0 || settings.http.maxRetries < 0
        || settings.http.retryDelayMs < 0 || settings.syncTimeoutMs <= 0
        || settings.syncIntervalMs <= 0) {
        error = QStringLiteral("Settings.json contains invalid AniList or HTTP settings.");
        return false;
    }
    if (settings.logRetentionDays <= 0) {
        error = QStringLiteral("Settings.json contains an invalid logging retention period.");
        return false;
    }

    return true;
}
