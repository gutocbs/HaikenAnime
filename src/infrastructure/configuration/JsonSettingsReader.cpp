#include "JsonSettingsReader.h"
#include "../../application/configuration/UserPreferencesValidator.h"

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

bool readDouble(const QJsonObject &object, const QString &key, const double defaultValue,
                double &result) {
    const auto value = object.value(key);
    if (value.isUndefined()) {
        result = defaultValue;
        return true;
    }
    if (!value.isDouble() || !std::isfinite(value.toDouble())) {
        return false;
    }
    result = value.toDouble();
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
    const auto covers = root.value(QStringLiteral("covers")).toObject();
    const auto userPreferencesValue = root.value(QStringLiteral("userPreferences"));
    if (!userPreferencesValue.isUndefined() && !userPreferencesValue.isObject()) {
        error = QStringLiteral("Settings.json userPreferences must be an object.");
        return false;
    }
    const auto userPreferences = userPreferencesValue.toObject();
    const auto scorePreferencesValue = userPreferences.value(QStringLiteral("score"));
    const auto coverPreferencesValue = userPreferences.value(QStringLiteral("covers"));
    const auto syncPreferencesValue = userPreferences.value(QStringLiteral("sync"));
    if ((!scorePreferencesValue.isUndefined() && !scorePreferencesValue.isObject())
        || (!coverPreferencesValue.isUndefined() && !coverPreferencesValue.isObject())
        || (!syncPreferencesValue.isUndefined() && !syncPreferencesValue.isObject())) {
        error = QStringLiteral("Settings.json user preference sections must be objects.");
        return false;
    }
    const auto scorePreferences = scorePreferencesValue.toObject();
    const auto coverPreferences = coverPreferencesValue.toObject();
    const auto syncPreferences = syncPreferencesValue.toObject();
    const auto coverQuality = covers.value(QStringLiteral("quality"));
    if (!coverQuality.isUndefined()) {
        if (!coverQuality.isString()) {
            error = QStringLiteral("Settings.json cover quality must be a string.");
            return false;
        }
        const auto parsedQuality = ParseCoverQuality(coverQuality.toString());
        if (!parsedQuality.has_value()) {
            error = QStringLiteral("Settings.json contains an unsupported cover quality.");
            return false;
        }
        settings.covers.quality = parsedQuality.value();
    }
    const auto userCoverQualityValue = coverPreferences.value(QStringLiteral("quality"));
    if (!userCoverQualityValue.isUndefined()) {
        if (!userCoverQualityValue.isString()) {
            error = QStringLiteral("Settings.json user cover quality must be a string.");
            return false;
        }
        const auto parsedQuality = ParseCoverQuality(userCoverQualityValue.toString());
        if (!parsedQuality.has_value()) {
            error = QStringLiteral("Settings.json contains an unsupported user cover quality.");
            return false;
        }
        settings.userPreferences.coverQuality = parsedQuality.value();
    }
    const auto syncEnabled = syncPreferences.value(QStringLiteral("enabled"));
    if (!syncEnabled.isUndefined()) {
        if (!syncEnabled.isBool()) {
            error = QStringLiteral("Settings.json user synchronization enabled must be boolean.");
            return false;
        }
        settings.userPreferences.synchronizationEnabled = syncEnabled.toBool();
    }
    if (!readInteger(http, QStringLiteral("timeoutMs"), 30000, settings.http.timeoutMs)
        || !readInteger(http, QStringLiteral("maxRetries"), 2, settings.http.maxRetries)
        || !readInteger(http, QStringLiteral("retryDelayMs"), 1000,
                        settings.http.retryDelayMs)
        || !readInteger(sync, QStringLiteral("timeoutMs"), 60000, settings.syncTimeoutMs)
        || !readInteger(sync, QStringLiteral("intervalMs"), 3600000,
                        settings.syncIntervalMs)
        || !readInteger(logging, QStringLiteral("retentionDays"), 7,
                        settings.logRetentionDays)
        || !readInteger(covers, QStringLiteral("maxConcurrentDownloads"), 3,
                        settings.covers.maxConcurrentDownloads)
        || !readInteger(covers, QStringLiteral("timeoutMs"), 30000,
                        settings.covers.timeoutMs)
        || !readInteger(covers, QStringLiteral("maxRetries"), 2,
                        settings.covers.maxRetries)
        || !readInteger(covers, QStringLiteral("retryDelayMs"), 1000,
                        settings.covers.retryDelayMs)
        || !readInteger(covers, QStringLiteral("maxResponseBytes"), 5 * 1024 * 1024,
                        settings.covers.maxResponseBytes)
        || !readInteger(covers, QStringLiteral("minDimension"), 32,
                        settings.covers.minDimension)
        || !readInteger(covers, QStringLiteral("maxDimension"), 4096,
                        settings.covers.maxDimension)
        || !readInteger(covers, QStringLiteral("failureCooldownMs"), 5 * 60 * 1000,
                        settings.covers.failureCooldownMs)
        || !readDouble(scorePreferences, QStringLiteral("minimum"), 0.0,
                       settings.userPreferences.scoreMinimum)
        || !readDouble(scorePreferences, QStringLiteral("maximum"), 10.0,
                       settings.userPreferences.scoreMaximum)
        || !readDouble(scorePreferences, QStringLiteral("step"), 1.0,
                       settings.userPreferences.scoreStep)
        || !readInteger(syncPreferences, QStringLiteral("intervalMs"), 3600000,
                        settings.userPreferences.synchronizationIntervalMs)) {
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
    if (settings.covers.maxConcurrentDownloads <= 0 || settings.covers.timeoutMs <= 0
        || settings.covers.maxRetries < 0 || settings.covers.retryDelayMs < 0
        || settings.covers.maxResponseBytes <= 0 || settings.covers.minDimension <= 0
        || settings.covers.maxDimension < settings.covers.minDimension
        || settings.covers.failureCooldownMs < 0) {
        error = QStringLiteral("Settings.json contains invalid cover settings.");
        return false;
    }
    const auto preferenceValidation = ValidateUserPreferences(settings.userPreferences);
    if (!preferenceValidation.valid) {
        error = QStringLiteral("Settings.json contains invalid user preferences: %1")
                    .arg(preferenceValidation.error);
        return false;
    }

    return true;
}
