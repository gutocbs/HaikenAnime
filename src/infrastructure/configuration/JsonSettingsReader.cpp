#include "JsonSettingsReader.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

JsonSettingsReader::JsonSettingsReader(QString filePath)
    : filePath_(std::move(filePath)) {
}

bool JsonSettingsReader::read(Settings &settings, QString &error) {
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
    settings.http.timeoutMs = http.value(QStringLiteral("timeoutMs")).toInt(30000);
    settings.http.maxRetries = http.value(QStringLiteral("maxRetries")).toInt(2);
    settings.http.retryDelayMs = http.value(QStringLiteral("retryDelayMs")).toInt(1000);
    const auto logging = root.value(QStringLiteral("logging")).toObject();
    settings.logRetentionDays = logging.value(QStringLiteral("retentionDays")).toInt(7);

    if (settings.aniList.endpoint.isEmpty() || settings.aniList.mediaQueryFile.isEmpty()
        || settings.http.timeoutMs <= 0 || settings.http.maxRetries < 0
        || settings.http.retryDelayMs < 0) {
        error = QStringLiteral("Settings.json contains invalid AniList or HTTP settings.");
        return false;
    }
    if (settings.logRetentionDays <= 0) {
        error = QStringLiteral("Settings.json contains an invalid logging retention period.");
        return false;
    }

    return true;
}
