#ifndef HAIKENANIME_SQLITEQUERYCONFIGURATION_H
#define HAIKENANIME_SQLITEQUERYCONFIGURATION_H

#include <QString>
class AsyncLogger;

struct SqliteQueryConfiguration final {
    QString upsertMediaPath;
    QString updatePersonalListMediaPath;
    QString readMediaPath;
    QString readActiveMediaIdsPath;
    QString markMediaSourceRemovedPath;
    QString enqueuePendingChangePath;
    QString readPendingChangesPath;
    QString updatePendingChangePath;
    QString readCoverCachePath;
    QString upsertCoverCachePath;
    QString deleteCoverCachePath;
    QString clearCoverCachePath;
    QString readUserPreferencesPath;
    QString upsertUserPreferencesPath;
    QString readSyncTaskStatesPath;
    QString upsertSyncTaskStatePath;
    QString deleteSyncTaskStatePath;

    QString beginLibraryScanPath;
    QString upsertLocalFilePath;
    QString completeLibraryScanPath;
    QString failLibraryScanPath;
    QString markLocalFilesUnavailablePath;
    QString readPendingLocalFilesPath;
    QString readCatalogMediaForRecognitionPath;
    QString saveLocalFileRecognitionPath;
    QString readNextLocalEpisodePath;
    QString readAvailableEpisodeCountPath;

    [[nodiscard]] bool load(QString &error,
                            const QString &configurationPath = QStringLiteral(":/sqlite/queries/sqlite-queries.json"));
    void setLogger(AsyncLogger *logger);
    AsyncLogger *logger = nullptr;
};

#endif
