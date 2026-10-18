#include "JsonSettingsReader.h"
#include "../../application/configuration/UserPreferencesValidator.h"

#include <QFile>
#include <QJsonArray>
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

bool readNonNegativeDuration(const QJsonObject &object, const QString &key,
                             std::chrono::milliseconds defaultValue,
                             std::chrono::milliseconds &result) {
    result = defaultValue;
    const auto value = object.value(key);
    if (value.isUndefined()) return true;
    if (!value.isDouble()) return false;
    const auto number = value.toDouble();
    if (std::trunc(number) != number || number < 0
        || number > static_cast<double>(std::numeric_limits<qint64>::max())) return false;
    result = std::chrono::milliseconds(static_cast<qint64>(number));
    return true;
}

bool readSyncTaskPolicies(const QJsonObject &sync, Settings &settings) {
    const auto policiesValue = sync.value(QStringLiteral("policies"));
    if (policiesValue.isUndefined()) return true;
    if (!policiesValue.isObject()) return false;
    const auto policies = policiesValue.toObject();
    for (const auto kind : {SyncTaskKind::UserList, SyncTaskKind::PendingChange, SyncTaskKind::ActiveCatalog,
                            SyncTaskKind::InactiveCatalog, SyncTaskKind::CompletedCatalog, SyncTaskKind::Cover,
                            SyncTaskKind::DerivedMetadata}) {
        const auto value = policies.value(ToString(kind));
        if (value.isUndefined()) continue;
        if (!value.isObject()) return false;
        auto &policy = settings.syncTaskPolicies.at(kind);
        const auto object = value.toObject();
        if (!readNonNegativeDuration(object, QStringLiteral("normalIntervalMs"), policy.normalInterval, policy.normalInterval)
            || !readNonNegativeDuration(object, QStringLiteral("staleProtectionTtlMs"), policy.staleProtectionTtl, policy.staleProtectionTtl)
            || !readNonNegativeDuration(object, QStringLiteral("initialRetryDelayMs"), policy.initialRetryDelay, policy.initialRetryDelay)
            || !readNonNegativeDuration(object, QStringLiteral("maximumRetryDelayMs"), policy.maximumRetryDelay, policy.maximumRetryDelay)
            || !readNonNegativeDuration(object, QStringLiteral("cooldownMs"), policy.cooldown, policy.cooldown)
            || !readDouble(object, QStringLiteral("jitterRatio"), policy.jitterRatio, policy.jitterRatio)
            || !readInteger(object, QStringLiteral("maximumConsecutiveImmediateRetries"), policy.maximumConsecutiveImmediateRetries,
                            policy.maximumConsecutiveImmediateRetries)
            || policy.jitterRatio < 0.0 || policy.jitterRatio > 1.0
            || policy.maximumConsecutiveImmediateRetries < 0
            || policy.maximumRetryDelay < policy.initialRetryDelay
            || policy.staleProtectionTtl < policy.normalInterval) return false;
    }
    return true;
}

template <typename Integer>
void readPositivePolicyInteger(const QJsonObject &object, const QString &key, const Integer defaultValue,
                               Integer &result) {
    result = defaultValue;
    const auto value = object.value(key);
    if (!value.isDouble()) return;
    const auto number = value.toDouble();
    if (std::trunc(number) != number || number <= 0 || number > std::numeric_limits<Integer>::max()) return;
    result = static_cast<Integer>(number);
}

bool readDocument(const QString &filePath, QJsonObject &root, QString &error) {
    QFile file(filePath);
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
    root = document.object();
    return true;
}

bool readAniList(const QJsonObject &aniList, AniListSettings &settings, QString &error) {
    settings.endpoint = aniList.value(QStringLiteral("endpoint")).toString();
    settings.mediaQueryFile = aniList.value(QStringLiteral("mediaQueryFile")).toString();
    settings.oauthClientId = aniList.value(QStringLiteral("oauthClientId")).toString();
    if (!readInteger(aniList, QStringLiteral("userListPerChunk"), 500, settings.userListPerChunk)) {
        error = QStringLiteral("Settings.json AniList user-list chunk size must be a whole number.");
        return false;
    }
    const auto oauthRedirectUri = aniList.value(QStringLiteral("oauthRedirectUri"));
    if (oauthRedirectUri.isUndefined()) return true;
    if (!oauthRedirectUri.isString()) {
        error = QStringLiteral("Settings.json AniList OAuth redirect URI must be a string.");
        return false;
    }
    settings.oauthRedirectUri = QUrl(oauthRedirectUri.toString());
    if (!settings.oauthRedirectUri.isValid() || settings.oauthRedirectUri.scheme().isEmpty()) {
        error = QStringLiteral("Settings.json AniList OAuth redirect URI is invalid.");
        return false;
    }
    return true;
}

/** The optional "userPreferences" object and its sections; every present section must be an object. */
struct UserPreferenceSections final {
    QJsonObject score;
    QJsonObject covers;
    QJsonObject sync;
    QJsonObject library;
};

bool readUserPreferenceSections(const QJsonObject &root, UserPreferenceSections &sections, QString &error) {
    const auto userPreferencesValue = root.value(QStringLiteral("userPreferences"));
    if (!userPreferencesValue.isUndefined() && !userPreferencesValue.isObject()) {
        error = QStringLiteral("Settings.json userPreferences must be an object.");
        return false;
    }
    const auto userPreferences = userPreferencesValue.toObject();
    const auto score = userPreferences.value(QStringLiteral("score"));
    const auto covers = userPreferences.value(QStringLiteral("covers"));
    const auto sync = userPreferences.value(QStringLiteral("sync"));
    const auto library = userPreferences.value(QStringLiteral("library"));
    for (const auto &section : {score, covers, sync, library}) {
        if (!section.isUndefined() && !section.isObject()) {
            error = QStringLiteral("Settings.json user preference sections must be objects.");
            return false;
        }
    }
    sections = {score.toObject(), covers.toObject(), sync.toObject(), library.toObject()};
    return true;
}

bool readStringArray(const QJsonValue &value, QStringList &strings) {
    if (!value.isArray()) return false;
    strings.clear();
    for (const auto &item : value.toArray()) {
        if (!item.isString()) return false;
        strings.append(item.toString());
    }
    return true;
}

bool readLibraryPreferences(const QJsonObject &library, UserPreferences &preferences, QString &error) {
    const auto libraryRoot = library.value(QStringLiteral("root"));
    if (!libraryRoot.isUndefined()) {
        if (!libraryRoot.isString()) {
            error = QStringLiteral("Settings.json library root must be a string.");
            return false;
        }
        preferences.libraryRoots = {libraryRoot.toString()};
    }
    // "roots" supersedes the legacy single "root" when both are present.
    const auto libraryRoots = library.value(QStringLiteral("roots"));
    if (!libraryRoots.isUndefined()) {
        QStringList roots;
        if (!readStringArray(libraryRoots, roots) || roots.isEmpty()) {
            error = QStringLiteral("Settings.json library roots must be a non-empty array of strings.");
            return false;
        }
        preferences.libraryRoots = roots;
    }
    const auto scanExtensions = library.value(QStringLiteral("extensions"));
    if (!scanExtensions.isUndefined()) {
        if (!scanExtensions.isArray()) {
            error = QStringLiteral("Settings.json scan extensions must be an array of strings.");
            return false;
        }
        QStringList extensions;
        if (!readStringArray(scanExtensions, extensions)) {
            error = QStringLiteral("Settings.json scan extensions must be strings.");
            return false;
        }
        preferences.scanExtensions = NormalizeScanExtensions(extensions, error);
        if (!error.isEmpty()) {
            error = QStringLiteral("Settings.json contains invalid scan extensions: %1").arg(error);
            return false;
        }
    }
    return true;
}

bool readCoverAndSyncPreferences(const UserPreferenceSections &sections, UserPreferences &preferences,
                                 QString &error) {
    const auto coverQuality = sections.covers.value(QStringLiteral("quality"));
    if (!coverQuality.isUndefined()) {
        if (!coverQuality.isString()) {
            error = QStringLiteral("Settings.json user cover quality must be a string.");
            return false;
        }
        const auto parsedQuality = ParseCoverQuality(coverQuality.toString());
        if (!parsedQuality.has_value()) {
            error = QStringLiteral("Settings.json contains an unsupported user cover quality.");
            return false;
        }
        preferences.coverQuality = parsedQuality.value();
    }
    const auto syncEnabled = sections.sync.value(QStringLiteral("enabled"));
    if (!syncEnabled.isUndefined()) {
        if (!syncEnabled.isBool()) {
            error = QStringLiteral("Settings.json user synchronization enabled must be boolean.");
            return false;
        }
        preferences.synchronizationEnabled = syncEnabled.toBool();
    }
    return true;
}

// Unlike the other sections, invalid seasonal catalog values silently fall back to their defaults.
void readSeasonalCatalog(const QJsonObject &seasonalCatalog, SeasonalCatalogCachePolicy &policy) {
    const SeasonalCatalogCachePolicy defaults;
    readPositivePolicyInteger(seasonalCatalog, QStringLiteral("cacheTtlMs"), defaults.timeToLiveMs,
                              policy.timeToLiveMs);
    readPositivePolicyInteger(seasonalCatalog, QStringLiteral("maxCacheEntries"), defaults.maximumEntries,
                              policy.maximumEntries);
    readPositivePolicyInteger(seasonalCatalog, QStringLiteral("minimumRequestIntervalMs"),
                              defaults.minimumRequestIntervalMs, policy.minimumRequestIntervalMs);
}

bool readNumbers(const QJsonObject &http, const QJsonObject &sync, const QJsonObject &logging,
                 const QJsonObject &covers, const UserPreferenceSections &sections, Settings &settings) {
    return readInteger(http, QStringLiteral("timeoutMs"), 30000, settings.http.timeoutMs)
        && readInteger(http, QStringLiteral("maxRetries"), 2, settings.http.maxRetries)
        && readInteger(http, QStringLiteral("retryDelayMs"), 1000, settings.http.retryDelayMs)
        && readInteger(sync, QStringLiteral("timeoutMs"), 60000, settings.syncTimeoutMs)
        && readInteger(logging, QStringLiteral("retentionDays"), 7, settings.logRetentionDays)
        && readInteger(covers, QStringLiteral("maxConcurrentDownloads"), 3, settings.covers.maxConcurrentDownloads)
        && readInteger(covers, QStringLiteral("timeoutMs"), 30000, settings.covers.timeoutMs)
        && readInteger(covers, QStringLiteral("maxRetries"), 2, settings.covers.maxRetries)
        && readInteger(covers, QStringLiteral("retryDelayMs"), 1000, settings.covers.retryDelayMs)
        && readInteger(covers, QStringLiteral("maxResponseBytes"), 5 * 1024 * 1024, settings.covers.maxResponseBytes)
        && readInteger(covers, QStringLiteral("minDimension"), 32, settings.covers.minDimension)
        && readInteger(covers, QStringLiteral("maxDimension"), 4096, settings.covers.maxDimension)
        && readInteger(covers, QStringLiteral("failureCooldownMs"), 5 * 60 * 1000, settings.covers.failureCooldownMs)
        && readDouble(sections.score, QStringLiteral("minimum"), 0.0, settings.userPreferences.scoreMinimum)
        && readDouble(sections.score, QStringLiteral("maximum"), 10.0, settings.userPreferences.scoreMaximum)
        && readDouble(sections.score, QStringLiteral("step"), 1.0, settings.userPreferences.scoreStep)
        && readInteger(sections.sync, QStringLiteral("intervalMs"), 3600000,
                       settings.userPreferences.synchronizationIntervalMs);
}

bool validateRanges(const Settings &settings, QString &error) {
    if (settings.aniList.endpoint.isEmpty() || settings.aniList.mediaQueryFile.isEmpty()
        || settings.aniList.userListPerChunk <= 0 || settings.aniList.userListPerChunk > 500
        || settings.http.timeoutMs <= 0 || settings.http.maxRetries < 0
        || settings.http.retryDelayMs < 0 || settings.syncTimeoutMs <= 0) {
        error = QStringLiteral("Settings.json contains invalid AniList or HTTP settings.");
        return false;
    }
    if (settings.logRetentionDays <= 0) {
        error = QStringLiteral("Settings.json contains an invalid logging retention period.");
        return false;
    }
    const auto &covers = settings.covers;
    if (covers.maxConcurrentDownloads <= 0 || covers.timeoutMs <= 0 || covers.maxRetries < 0
        || covers.retryDelayMs < 0 || covers.maxResponseBytes <= 0 || covers.minDimension <= 0
        || covers.maxDimension < covers.minDimension || covers.failureCooldownMs < 0) {
        error = QStringLiteral("Settings.json contains invalid cover settings.");
        return false;
    }
    const auto preferenceValidation = ValidateUserPreferences(settings.userPreferences);
    if (!preferenceValidation.valid) {
        error = QStringLiteral("Settings.json contains invalid user preferences: %1").arg(preferenceValidation.error);
        return false;
    }
    return true;
}
}

JsonSettingsReader::JsonSettingsReader(QString filePath)
    : filePath_(std::move(filePath)) {
}

bool JsonSettingsReader::read(Settings &settings, QString &error) {
    settings = {};
    error.clear();
    QJsonObject root;
    if (!readDocument(filePath_, root, error)) return false;

    const auto aniList = root.value(QStringLiteral("anilist")).toObject();
    const auto http = root.value(QStringLiteral("http")).toObject();
    if (aniList.isEmpty() || http.isEmpty()) {
        error = QStringLiteral("Settings.json must contain anilist and http objects.");
        return false;
    }
    if (!readAniList(aniList, settings.aniList, error)) return false;

    const auto sync = root.value(QStringLiteral("sync")).toObject();
    const auto logging = root.value(QStringLiteral("logging")).toObject();
    const auto covers = root.value(QStringLiteral("covers")).toObject();
    UserPreferenceSections preferenceSections;
    if (!readUserPreferenceSections(root, preferenceSections, error)) return false;
    if (!readSyncTaskPolicies(sync, settings)) {
        error = QStringLiteral("Settings.json contains invalid synchronization task policy settings.");
        return false;
    }
    if (!readLibraryPreferences(preferenceSections.library, settings.userPreferences, error)
        || !readCoverAndSyncPreferences(preferenceSections, settings.userPreferences, error)) {
        return false;
    }
    readSeasonalCatalog(root.value(QStringLiteral("seasonalCatalog")).toObject(),
                        settings.seasonalCatalogCachePolicy);
    if (!readNumbers(http, sync, logging, covers, preferenceSections, settings)) {
        error = QStringLiteral("Settings.json numeric settings must contain whole numbers.");
        return false;
    }
    return validateRanges(settings, error);
}
