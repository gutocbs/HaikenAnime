#ifndef HAIKENANIME_SQLITEMEDIAMAPPER_H
#define HAIKENANIME_SQLITEMEDIAMAPPER_H

#include <QSqlQuery>

#include "../../domain/media/Media.h"

class SqliteMediaMapper final {
public:
    static Media Map(const QSqlQuery &query);
};

#endif // HAIKENANIME_SQLITEMEDIAMAPPER_H
