#ifndef HAIKENANIME_SQLITEQUERYCONFIGURATION_H
#define HAIKENANIME_SQLITEQUERYCONFIGURATION_H

#include <QString>
class AsyncLogger;

struct SqliteQueryConfiguration final {
    QString upsertMediaPath;
    QString readMediaPath;

    [[nodiscard]] bool load(QString &error) ;
    void setLogger(AsyncLogger *logger);
    AsyncLogger *logger = nullptr;
};

#endif
