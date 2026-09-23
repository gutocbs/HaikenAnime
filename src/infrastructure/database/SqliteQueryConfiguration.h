#ifndef HAIKENANIME_SQLITEQUERYCONFIGURATION_H
#define HAIKENANIME_SQLITEQUERYCONFIGURATION_H

#include <QString>

struct SqliteQueryConfiguration final {
    QString upsertMediaPath;
    QString readMediaPath;

    [[nodiscard]] bool load(QString &error) ;
};

#endif
