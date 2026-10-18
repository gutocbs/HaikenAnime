#include "SqliteMediaProgressReader.h"

#include <QSqlError>
#include <QSqlQuery>

#include <utility>

#include "SqlQueryStore.h"
#include "SqliteMediaMapper.h"

SqliteMediaProgressReader::SqliteMediaProgressReader(QSqlDatabase database, QString readQuery)
    : database_(std::move(database)), readQuery_(std::move(readQuery)) {}

bool SqliteMediaProgressReader::find(const int mediaId, std::optional<Media> &media, QString &error) {
    media.reset();
    error.clear();
    if (!database_.isOpen()) {
        error = QStringLiteral("SQLite database is not open.");
        return false;
    }
    QString sql;
    if (!SqlQueryStore::loadSource(readQuery_, sql, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(sql)) {
        error = query.lastError().text();
        return false;
    }
    query.bindValue(QStringLiteral(":media_id"), mediaId);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    if (query.next()) media = SqliteMediaMapper::Map(query);
    return true;
}
