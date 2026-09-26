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
        "cover_medium_url TEXT,"
        "cover_large_url TEXT,"
        "cover_extra_large_url TEXT,"
        "synopsis TEXT,"
        "type INTEGER NOT NULL,"
        "status INTEGER NOT NULL,"
        "user_list_status INTEGER NOT NULL DEFAULT -1,"
        "source_removed_at TEXT"
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
    const bool userPreferencesCreated = coverCacheCreated && query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS user_preferences ("
        "id INTEGER PRIMARY KEY CHECK (id = 1),"
        "score_minimum REAL NOT NULL,"
        "score_maximum REAL NOT NULL,"
        "score_step REAL NOT NULL,"
        "cover_quality TEXT NOT NULL,"
        "synchronization_enabled INTEGER NOT NULL CHECK (synchronization_enabled IN (0, 1)),"
        "synchronization_interval_ms INTEGER NOT NULL)"));
    bool sourceRemovalColumnExists = false;
    bool userListStatusColumnExists = false;
    bool coverMediumColumnExists = false;
    bool coverLargeColumnExists = false;
    bool coverExtraLargeColumnExists = false;
    bool sourceRemovalReady = false;
    bool userListStatusReady = false;
    if (userPreferencesCreated && query.exec(QStringLiteral("PRAGMA table_info(media)"))) {
        while (query.next()) {
            const auto column = query.value(1).toString();
            if (column == QStringLiteral("source_removed_at")) sourceRemovalColumnExists = true;
            if (column == QStringLiteral("user_list_status")) userListStatusColumnExists = true;
            if (column == QStringLiteral("cover_medium_url")) coverMediumColumnExists = true;
            if (column == QStringLiteral("cover_large_url")) coverLargeColumnExists = true;
            if (column == QStringLiteral("cover_extra_large_url")) coverExtraLargeColumnExists = true;
        }
        sourceRemovalReady = sourceRemovalColumnExists
            || query.exec(QStringLiteral("ALTER TABLE media ADD COLUMN source_removed_at TEXT"));
        userListStatusReady = userListStatusColumnExists
            || query.exec(QStringLiteral(
                "ALTER TABLE media ADD COLUMN user_list_status INTEGER NOT NULL DEFAULT -1"));
    }
    const bool coverMediumReady = coverMediumColumnExists
        || query.exec(QStringLiteral("ALTER TABLE media ADD COLUMN cover_medium_url TEXT"));
    const bool coverLargeReady = coverMediumReady && (coverLargeColumnExists
        || query.exec(QStringLiteral("ALTER TABLE media ADD COLUMN cover_large_url TEXT")));
    const bool coverExtraLargeReady = coverLargeReady && (coverExtraLargeColumnExists
        || query.exec(QStringLiteral("ALTER TABLE media ADD COLUMN cover_extra_large_url TEXT")));
    const bool versionInserted = sourceRemovalReady && userListStatusReady && coverExtraLargeReady && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (1)"));
    const bool coverVersionInserted = versionInserted && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (2)"));
    const bool sourceRemovalVersionInserted = coverVersionInserted && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (3)"));
    const bool userListStatusVersionInserted = sourceRemovalVersionInserted && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (4)"));
    const bool userPreferencesVersionInserted = userListStatusVersionInserted && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (5)"));
    const bool coverVariantsVersionInserted = userPreferencesVersionInserted && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (6)"));
    const bool versionQueried = coverVariantsVersionInserted && query.exec(QStringLiteral(
        "SELECT EXISTS(SELECT 1 FROM schema_version WHERE version = 1), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 2), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 3), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 4), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 5), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 6)"));
    const bool versionRecorded = versionQueried && query.next() && query.value(0).toBool()
        && query.value(1).toBool() && query.value(2).toBool() && query.value(3).toBool()
        && query.value(4).toBool() && query.value(5).toBool();

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
        lastError_ = QStringLiteral("SQLite migration versions 1 through 6 were not recorded.");
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
