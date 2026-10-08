#include "SqliteQueryConfiguration.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include "../logging/AsyncLogger.h"

void SqliteQueryConfiguration::setLogger(AsyncLogger *value) { logger = value; }

bool SqliteQueryConfiguration::load(QString &error, const QString &configurationPath) {
    upsertMediaPath.clear();
    updatePersonalListMediaPath.clear();
    readMediaPath.clear();
    readActiveMediaIdsPath.clear();
    markMediaSourceRemovedPath.clear();
    enqueuePendingChangePath.clear();
    readPendingChangesPath.clear();
    updatePendingChangePath.clear();
    readCoverCachePath.clear();
    upsertCoverCachePath.clear();
    deleteCoverCachePath.clear();
    clearCoverCachePath.clear();
    readUserPreferencesPath.clear();
    upsertUserPreferencesPath.clear();
    readSyncTaskStatesPath.clear();
    upsertSyncTaskStatePath.clear();
    deleteSyncTaskStatePath.clear();
    beginLibraryScanPath.clear();
    upsertLocalFilePath.clear();
    completeLibraryScanPath.clear();
    failLibraryScanPath.clear();
    markLocalFilesUnavailablePath.clear();
    readPendingLocalFilesPath.clear();
    readCatalogMediaForRecognitionPath.clear();
    saveLocalFileRecognitionPath.clear();
    readNextLocalEpisodePath.clear();
    readAvailableEpisodeCountPath.clear();
    error.clear();
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
    updatePersonalListMediaPath = queries.value(QStringLiteral("updatePersonalListMedia")).toString();
    readMediaPath = queries.value(QStringLiteral("readMedia")).toString();
    readActiveMediaIdsPath = queries.value(QStringLiteral("readActiveMediaIds")).toString();
    markMediaSourceRemovedPath = queries.value(QStringLiteral("markMediaSourceRemoved")).toString();
    enqueuePendingChangePath = queries.value(QStringLiteral("enqueuePendingChange")).toString();
    readPendingChangesPath = queries.value(QStringLiteral("readPendingChanges")).toString();
    updatePendingChangePath = queries.value(QStringLiteral("updatePendingChange")).toString();
    readCoverCachePath = queries.value(QStringLiteral("readCoverCache")).toString();
    upsertCoverCachePath = queries.value(QStringLiteral("upsertCoverCache")).toString();
    deleteCoverCachePath = queries.value(QStringLiteral("deleteCoverCache")).toString();
    clearCoverCachePath = queries.value(QStringLiteral("clearCoverCache")).toString();
    readUserPreferencesPath = queries.value(QStringLiteral("readUserPreferences")).toString();
    upsertUserPreferencesPath = queries.value(QStringLiteral("upsertUserPreferences")).toString();
    readSyncTaskStatesPath = queries.value(QStringLiteral("readSyncTaskStates")).toString();
    upsertSyncTaskStatePath = queries.value(QStringLiteral("upsertSyncTaskState")).toString();
    deleteSyncTaskStatePath = queries.value(QStringLiteral("deleteSyncTaskState")).toString();
    beginLibraryScanPath = queries.value(QStringLiteral("beginLibraryScan")).toString();
    upsertLocalFilePath = queries.value(QStringLiteral("upsertLocalFile")).toString();
    completeLibraryScanPath = queries.value(QStringLiteral("completeLibraryScan")).toString();
    failLibraryScanPath = queries.value(QStringLiteral("failLibraryScan")).toString();
    markLocalFilesUnavailablePath = queries.value(QStringLiteral("markLocalFilesUnavailable")).toString();
    readPendingLocalFilesPath = queries.value(QStringLiteral("readPendingLocalFiles")).toString();
    readCatalogMediaForRecognitionPath = queries.value(QStringLiteral("readCatalogMediaForRecognition")).toString();
    saveLocalFileRecognitionPath = queries.value(QStringLiteral("saveLocalFileRecognition")).toString();
    readNextLocalEpisodePath = queries.value(QStringLiteral("readNextLocalEpisode")).toString();
    readAvailableEpisodeCountPath = queries.value(QStringLiteral("readAvailableEpisodeCount")).toString();
    if (upsertMediaPath.isEmpty() || updatePersonalListMediaPath.isEmpty() || readMediaPath.isEmpty() || readActiveMediaIdsPath.isEmpty()
        || markMediaSourceRemovedPath.isEmpty() || enqueuePendingChangePath.isEmpty()
        || readPendingChangesPath.isEmpty() || updatePendingChangePath.isEmpty()
        || readCoverCachePath.isEmpty() || upsertCoverCachePath.isEmpty()
        || deleteCoverCachePath.isEmpty() || clearCoverCachePath.isEmpty()
        || readUserPreferencesPath.isEmpty() || upsertUserPreferencesPath.isEmpty()
        || readSyncTaskStatesPath.isEmpty() || upsertSyncTaskStatePath.isEmpty() || deleteSyncTaskStatePath.isEmpty()
        || beginLibraryScanPath.isEmpty() || upsertLocalFilePath.isEmpty()
        || completeLibraryScanPath.isEmpty() || failLibraryScanPath.isEmpty()
        || markLocalFilesUnavailablePath.isEmpty() || readPendingLocalFilesPath.isEmpty()
        || readCatalogMediaForRecognitionPath.isEmpty() || saveLocalFileRecognitionPath.isEmpty()
        || readNextLocalEpisodePath.isEmpty() || readAvailableEpisodeCountPath.isEmpty()) {
        error = QStringLiteral("SQLite query configuration is incomplete.");
        if (logger) logger->error(LogCategory::QueryConfiguration, error);
        return false;
    }
    if (logger) logger->info(LogCategory::QueryConfiguration, QStringLiteral("SQLite query configuration loaded."));
    return true;
}
