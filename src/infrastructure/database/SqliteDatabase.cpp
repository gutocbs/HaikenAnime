#include "SqliteDatabase.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

#include <utility>

SqliteDatabase::SqliteDatabase(QString databasePath)
    : connectionName_(QStringLiteral("haikenanime-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces))),
      databasePath_(std::move(databasePath)) {
    if (databasePath_.isEmpty()) {
        const auto appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        databasePath_ = QDir(appDataPath).filePath(QStringLiteral("haikenanime.sqlite"));
    }
}

SqliteDatabase::~SqliteDatabase() {
    close();
}

bool SqliteDatabase::open() {
    if (database_.isOpen()) {
        return true;
    }

    const QFileInfo fileInfo(databasePath_);
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        return false;
    }

    database_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    database_.setDatabaseName(databasePath_);
    return database_.open();
}

bool SqliteDatabase::migrate() {
    if (!database_.isOpen()) {
        return false;
    }

    if (!database_.transaction()) {
        return false;
    }

    QSqlQuery query(database_);
    const bool schemaCreated = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS schema_version ("
        "version INTEGER PRIMARY KEY"
        ")"));
    const bool mediaCreated = schemaCreated && query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS media ("
        "id INTEGER PRIMARY KEY,"
        "name TEXT NOT NULL,"
        "english_name TEXT,"
        "original_name TEXT,"
        "alternative_names TEXT NOT NULL DEFAULT '[]',"
        "total_chapters INTEGER NOT NULL DEFAULT 0,"
        "consumed_chapters INTEGER NOT NULL DEFAULT 0,"
        "next_chapter INTEGER NOT NULL DEFAULT 0,"
        "average_score INTEGER NOT NULL DEFAULT 0,"
        "personal_score INTEGER NOT NULL DEFAULT 0,"
        "cover_url TEXT,"
        "synopsis TEXT,"
        "type INTEGER NOT NULL,"
        "status INTEGER NOT NULL"
        ")"));
    const bool pendingChangesCreated = mediaCreated && query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS anilist_pending_changes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "media_id INTEGER NOT NULL,"
        "field INTEGER NOT NULL,"
        "previous_value TEXT NOT NULL,"
        "new_value TEXT NOT NULL,"
        "created_at TEXT NOT NULL,"
        "attempts INTEGER NOT NULL DEFAULT 0,"
        "status INTEGER NOT NULL,"
        "last_error TEXT NOT NULL DEFAULT '',"
        "FOREIGN KEY(media_id) REFERENCES media(id)"
        ")"));
    const bool versionRecorded = pendingChangesCreated && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (1)"));

    if (versionRecorded) {
        return database_.commit();
    }

    database_.rollback();
    return false;
}

void SqliteDatabase::close() {
    if (database_.isValid()) {
        const auto connectionName = database_.connectionName();
        database_.close();
        database_ = {};
        QSqlDatabase::removeDatabase(connectionName);
    }
}

bool SqliteDatabase::isOpen() const {
    return database_.isOpen();
}

QString SqliteDatabase::lastError() const {
    return database_.lastError().text();
}

QString SqliteDatabase::databasePath() const {
    return databasePath_;
}

QSqlDatabase SqliteDatabase::connection() const {
    return database_;
}
