#include "SqliteMediaProgressReader.h"

#include <QSqlError>
#include <QSqlQuery>

#include <utility>

#include "SqliteQueries.h"
#include "SqlQueryStore.h"
#include "SqliteMediaMapper.h"

SqliteMediaProgressReader::SqliteMediaProgressReader(QSqlDatabase database) : database_(std::move(database)) {}

bool SqliteMediaProgressReader::find(const int mediaId, std::optional<Media> &media, QString &error) {
    media.reset();
    error.clear();
    if (!database_.isOpen()) {
        error = QStringLiteral("SQLite database is not open.");
        return false;
    }
    QString sql;
    if (!SqlQueryStore::loadSource(SqliteQueries::ReadMediaForRecognitionProgress, sql, error)) return false;
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
