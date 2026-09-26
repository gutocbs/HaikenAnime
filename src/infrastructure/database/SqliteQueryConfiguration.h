#ifndef HAIKENANIME_SQLITEQUERYCONFIGURATION_H
#define HAIKENANIME_SQLITEQUERYCONFIGURATION_H

#include <QString>
class AsyncLogger;

struct SqliteQueryConfiguration final {
    QString upsertMediaPath;
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

    [[nodiscard]] bool load(QString &error) ;
    void setLogger(AsyncLogger *logger);
    AsyncLogger *logger = nullptr;
};

#endif
