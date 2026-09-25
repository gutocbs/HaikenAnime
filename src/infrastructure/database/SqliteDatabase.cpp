#include "SqliteDatabase.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include "../logging/AsyncLogger.h"

#include <utility>

SqliteDatabase::SqliteDatabase(QString databasePath)
    : connectionName_(QStringLiteral("haikenanime-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces))),
      databasePath_(std::move(databasePath)) {
    if (databasePath_.isEmpty()) {
        const auto appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        databasePath_ = QDir(appDataPath).filePath(QStringLiteral("haikenanime.sqlite"));
    }
}

void SqliteDatabase::setLogger(AsyncLogger *logger) { logger_ = logger; }

SqliteDatabase::~SqliteDatabase() {
    close();
}

bool SqliteDatabase::open() {
    lastError_.clear();
    if (database_.isOpen()) {
        return true;
    }

    const QFileInfo fileInfo(databasePath_);
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        lastError_ = QStringLiteral("Could not create the SQLite database directory '%1'.")
                         .arg(fileInfo.absolutePath());
        return false;
    }

    database_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    database_.setDatabaseName(databasePath_);
    bool opened = database_.open();
    if (!opened) {
        lastError_ = database_.lastError().text();
    }
    if (opened) {
        QSqlQuery pragma(database_);
        opened = pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
        if (!opened) {
            lastError_ = pragma.lastError().text();
            database_.close();
        }
    }
    if (logger_) {
        (opened ? logger_->info(LogCategory::Database, QStringLiteral("SQLite database opened: %1").arg(databasePath_))
                : logger_->error(LogCategory::Database, database_.lastError().text()));
    }
    return opened;
}

bool SqliteDatabase::migrate() {
    lastError_.clear();
    if (!database_.isOpen()) {
        lastError_ = QStringLiteral("SQLite database is not open.");
        return false;
    }

    if (!database_.transaction()) {
        lastError_ = database_.lastError().text();
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
        "local_updated_at TEXT NOT NULL,"
        "remote_observed_at TEXT,"
        "remote_version TEXT NOT NULL DEFAULT '',"
        "attempts INTEGER NOT NULL DEFAULT 0,"
        "status INTEGER NOT NULL,"
        "last_error TEXT NOT NULL DEFAULT '',"
        "FOREIGN KEY(media_id) REFERENCES media(id)"
        ")"));
    const bool coverCacheCreated = pendingChangesCreated && query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS cover_cache ("
        "media_id INTEGER PRIMARY KEY,"
        "remote_url TEXT NOT NULL,"
        "quality TEXT NOT NULL,"
        "relative_path TEXT NOT NULL,"
        "mime_type TEXT NOT NULL,"
        "byte_size INTEGER NOT NULL,"
        "etag TEXT NOT NULL DEFAULT '',"
        "last_modified TEXT NOT NULL DEFAULT '',"
        "validated_at TEXT NOT NULL,"
        "FOREIGN KEY(media_id) REFERENCES media(id) ON DELETE CASCADE"
        ")"));
    const bool versionInserted = coverCacheCreated && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (1)"));
    const bool coverVersionInserted = versionInserted && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (2)"));
    const bool versionQueried = coverVersionInserted && query.exec(QStringLiteral(
        "SELECT EXISTS(SELECT 1 FROM schema_version WHERE version = 1), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 2)"));
    const bool versionRecorded = versionQueried && query.next() && query.value(0).toBool()
        && query.value(1).toBool();

    if (versionRecorded) {
        const bool committed = database_.commit();
        if (!committed) {
            lastError_ = database_.lastError().text();
        }
        if (logger_) {
            committed ? logger_->info(LogCategory::Migration, QStringLiteral("SQLite migration completed."))
                      : logger_->error(LogCategory::Migration, database_.lastError().text());
        }
        return committed;
    }

    lastError_ = query.lastError().text();
    if (lastError_.isEmpty()) {
        lastError_ = QStringLiteral("SQLite migration versions 1 and 2 were not recorded.");
    }
    database_.rollback();
    if (logger_) logger_->error(LogCategory::Migration, query.lastError().text());
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
    return lastError_;
}

QString SqliteDatabase::databasePath() const {
    return databasePath_;
}

QSqlDatabase SqliteDatabase::connection() const {
    return database_;
}
