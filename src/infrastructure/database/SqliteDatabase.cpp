#include "SqliteDatabase.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QSet>
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
        "local_path TEXT NOT NULL DEFAULT '',"
        "source_removed_at TEXT,"
        "season TEXT,"
        "season_year INTEGER,"
        "next_airing_episode INTEGER,"
        "next_airing_at INTEGER,"
        "anilist_url TEXT,"
        "external_links TEXT NOT NULL DEFAULT '[]'"
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
        "synchronization_interval_ms INTEGER NOT NULL,"
        "home_sort_key TEXT NOT NULL DEFAULT 'title_asc',"
        "card_status_presentation TEXT NOT NULL DEFAULT 'personal-list-status',"
        "language_key TEXT NOT NULL DEFAULT 'pt-BR',"
        "preferred_title_key TEXT NOT NULL DEFAULT 'romaji',"
        "include_adult_content INTEGER NOT NULL DEFAULT 0 CHECK (include_adult_content IN (0, 1)),"
        "automatic_local_file_recognition INTEGER NOT NULL DEFAULT 1 CHECK (automatic_local_file_recognition IN (0, 1)))"));
    bool sourceRemovalColumnExists = false;
    bool userListStatusColumnExists = false;
    bool coverMediumColumnExists = false;
    bool coverLargeColumnExists = false;
    bool coverExtraLargeColumnExists = false;
    bool seasonColumnExists = false;
    bool seasonYearColumnExists = false;
    bool nextAiringEpisodeColumnExists = false;
    bool nextAiringAtColumnExists = false;
    bool aniListUrlColumnExists = false;
    bool externalLinksColumnExists = false;
    bool localPathColumnExists = false;
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
            if (column == QStringLiteral("season")) seasonColumnExists = true;
            if (column == QStringLiteral("season_year")) seasonYearColumnExists = true;
            if (column == QStringLiteral("next_airing_episode")) nextAiringEpisodeColumnExists = true;
            if (column == QStringLiteral("next_airing_at")) nextAiringAtColumnExists = true;
            if (column == QStringLiteral("anilist_url")) aniListUrlColumnExists = true;
            if (column == QStringLiteral("external_links")) externalLinksColumnExists = true;
            if (column == QStringLiteral("local_path")) localPathColumnExists = true;
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
    const bool libraryScansCreated = coverVariantsVersionInserted && query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS library_scans ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "root_path TEXT NOT NULL,"
        "started_at TEXT NOT NULL,"
        "finished_at TEXT,"
        "status TEXT NOT NULL CHECK (status IN ('running', 'succeeded', 'failed', 'interrupted')),"
        "observed_count INTEGER NOT NULL DEFAULT 0 CHECK (observed_count >= 0),"
        "diagnostic TEXT NOT NULL DEFAULT '')"));
    const bool localFilesCreated = libraryScansCreated && query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS local_files ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "root_path TEXT NOT NULL,"
        "relative_path TEXT NOT NULL,"
        "normalized_relative_path TEXT NOT NULL,"
        "file_name TEXT NOT NULL,"
        "extension TEXT NOT NULL,"
        "size_bytes INTEGER NOT NULL CHECK (size_bytes >= 0),"
        "modified_at TEXT NOT NULL,"
        "available INTEGER NOT NULL DEFAULT 1 CHECK (available IN (0, 1)),"
        "last_seen_scan_id INTEGER NOT NULL REFERENCES library_scans(id),"
        "recognition_state TEXT NOT NULL DEFAULT 'unprocessed' "
        "CHECK (recognition_state IN ('unprocessed', 'recognized', 'unrecognized', 'ambiguous', 'associated', 'unsupported')) ,"
        "extracted_title TEXT NOT NULL DEFAULT '',"
        "media_kind TEXT NOT NULL DEFAULT 'anime',"
        "season INTEGER, episode INTEGER, media_id INTEGER REFERENCES media(id),"
        "recognition_diagnostic TEXT NOT NULL DEFAULT '', recognized_at TEXT,"
        "UNIQUE(root_path, normalized_relative_path))"));
    const bool inventoryVersionInserted = localFilesCreated && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (7)"));
    bool libraryRootExists = false;
    bool scanExtensionsExists = false;
    bool homeSortKeyExists = false;
    bool scannerPreferencesReady = false;
    if (inventoryVersionInserted && query.exec(QStringLiteral("PRAGMA table_info(user_preferences)"))) {
        while (query.next()) {
            const auto column = query.value(1).toString();
            if (column == QStringLiteral("library_root")) libraryRootExists = true;
            if (column == QStringLiteral("scan_extensions")) scanExtensionsExists = true;
            if (column == QStringLiteral("home_sort_key")) homeSortKeyExists = true;
        }
        scannerPreferencesReady = (libraryRootExists || query.exec(QStringLiteral(
            "ALTER TABLE user_preferences ADD COLUMN library_root TEXT NOT NULL DEFAULT 'Q:\\'")))
            && (scanExtensionsExists || query.exec(QStringLiteral(
            "ALTER TABLE user_preferences ADD COLUMN scan_extensions TEXT NOT NULL DEFAULT "
            "'[\".mkv\",\".mp4\",\".avi\",\".webm\",\".m4v\",\".mov\",\".wmv\",\".ts\"]'")));
    }
    const bool scannerPreferencesVersionInserted = scannerPreferencesReady && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (8)"));
    const bool homeSortKeyReady = scannerPreferencesVersionInserted && (homeSortKeyExists || query.exec(QStringLiteral(
        "ALTER TABLE user_preferences ADD COLUMN home_sort_key TEXT NOT NULL DEFAULT 'title_asc'")));
    const bool homeSortKeyVersionInserted = homeSortKeyReady && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (9)"));
    bool cardStatusPresentationExists = false;
    bool cardStatusPresentationReady = false;
    if (homeSortKeyVersionInserted && query.exec(QStringLiteral("PRAGMA table_info(user_preferences)"))) {
        while (query.next()) {
            if (query.value(1).toString() == QStringLiteral("card_status_presentation")) {
                cardStatusPresentationExists = true;
            }
        }
        cardStatusPresentationReady = cardStatusPresentationExists || query.exec(QStringLiteral(
            "ALTER TABLE user_preferences ADD COLUMN card_status_presentation TEXT NOT NULL "
            "DEFAULT 'personal-list-status'"));
    }
    const bool cardStatusPresentationVersionInserted = cardStatusPresentationReady && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (10)"));
    bool languageKeyExists = false;
    bool languageKeyReady = false;
    if (cardStatusPresentationVersionInserted && query.exec(QStringLiteral("PRAGMA table_info(user_preferences)"))) {
        while (query.next()) {
            if (query.value(1).toString() == QStringLiteral("language_key")) languageKeyExists = true;
        }
        languageKeyReady = languageKeyExists || query.exec(QStringLiteral(
            "ALTER TABLE user_preferences ADD COLUMN language_key TEXT NOT NULL DEFAULT 'pt-BR'"));
    }
    const bool languageVersionInserted = languageKeyReady && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (11)"));
    bool preferredTitleKeyExists = false;
    bool preferredTitleKeyReady = false;
    if (languageVersionInserted && query.exec(QStringLiteral("PRAGMA table_info(user_preferences)"))) {
        while (query.next()) {
            if (query.value(1).toString() == QStringLiteral("preferred_title_key")) {
                preferredTitleKeyExists = true;
            }
        }
        preferredTitleKeyReady = preferredTitleKeyExists || query.exec(QStringLiteral(
            "ALTER TABLE user_preferences ADD COLUMN preferred_title_key TEXT NOT NULL DEFAULT 'romaji'"));
    }
    const bool preferredTitleVersionInserted = preferredTitleKeyReady && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (12)"));
    const bool seasonReady = preferredTitleVersionInserted && (seasonColumnExists
        || query.exec(QStringLiteral("ALTER TABLE media ADD COLUMN season TEXT")));
    const bool seasonYearReady = seasonReady && (seasonYearColumnExists
        || query.exec(QStringLiteral("ALTER TABLE media ADD COLUMN season_year INTEGER")));
    const bool nextAiringEpisodeReady = seasonYearReady && (nextAiringEpisodeColumnExists
        || query.exec(QStringLiteral("ALTER TABLE media ADD COLUMN next_airing_episode INTEGER")));
    const bool nextAiringAtReady = nextAiringEpisodeReady && (nextAiringAtColumnExists
        || query.exec(QStringLiteral("ALTER TABLE media ADD COLUMN next_airing_at INTEGER")));
    const bool aniListUrlReady = nextAiringAtReady && (aniListUrlColumnExists
        || query.exec(QStringLiteral("ALTER TABLE media ADD COLUMN anilist_url TEXT")));
    const bool externalLinksReady = aniListUrlReady && (externalLinksColumnExists
        || query.exec(QStringLiteral(
            "ALTER TABLE media ADD COLUMN external_links TEXT NOT NULL DEFAULT '[]'")));
    const bool extendedMediaVersionInserted = externalLinksReady && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (13)"));
    bool includeAdultContentExists = false;
    bool adultContentReady = false;
    if (extendedMediaVersionInserted && query.exec(QStringLiteral("PRAGMA table_info(user_preferences)"))) {
        while (query.next()) {
            if (query.value(1).toString() == QStringLiteral("include_adult_content")) {
                includeAdultContentExists = true;
            }
        }
        adultContentReady = includeAdultContentExists || query.exec(QStringLiteral(
            "ALTER TABLE user_preferences ADD COLUMN include_adult_content INTEGER NOT NULL "
            "DEFAULT 0 CHECK (include_adult_content IN (0, 1))"));
    }
    const bool adultContentVersionInserted = adultContentReady && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (14)"));
    const bool localPathReady = adultContentVersionInserted && (localPathColumnExists
        || query.exec(QStringLiteral("ALTER TABLE media ADD COLUMN local_path TEXT NOT NULL DEFAULT ''")));
    const bool localPathVersionInserted = localPathReady && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (15)"));
    bool recognitionFieldsReady = false;
    if (localPathVersionInserted && query.exec(QStringLiteral("PRAGMA table_info(local_files)"))) {
        QSet<QString> columns;
        while (query.next()) columns.insert(query.value(1).toString());
        const auto addColumn = [&query, &columns](const QString &name, const QString &definition) {
            if (columns.contains(name)) return true;
            if (!query.exec(QStringLiteral("ALTER TABLE local_files ADD COLUMN ") + definition)) return false;
            columns.insert(name);
            return true;
        };
        recognitionFieldsReady = addColumn(QStringLiteral("extracted_title"), QStringLiteral("extracted_title TEXT NOT NULL DEFAULT ''"))
            && addColumn(QStringLiteral("media_kind"), QStringLiteral("media_kind TEXT NOT NULL DEFAULT 'anime'"))
            && addColumn(QStringLiteral("season"), QStringLiteral("season INTEGER"))
            && addColumn(QStringLiteral("episode"), QStringLiteral("episode INTEGER"))
            && addColumn(QStringLiteral("media_id"), QStringLiteral("media_id INTEGER"))
            && addColumn(QStringLiteral("recognition_diagnostic"), QStringLiteral("recognition_diagnostic TEXT NOT NULL DEFAULT ''"))
            && addColumn(QStringLiteral("recognized_at"), QStringLiteral("recognized_at TEXT"));
    }
    const bool recognitionVersionInserted = recognitionFieldsReady && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (16)"));
    bool recognitionStateConstraintReady = false;
    if (recognitionVersionInserted && query.exec(QStringLiteral(
            "SELECT sql FROM sqlite_master WHERE type = 'table' AND name = 'local_files'")) && query.next()) {
        recognitionStateConstraintReady = query.value(0).toString().contains(QStringLiteral("'ambiguous'"));
    }
    if (recognitionVersionInserted && !recognitionStateConstraintReady) {
        const bool legacyLocalFilesRenamed = query.exec(QStringLiteral(
            "ALTER TABLE local_files RENAME TO local_files_legacy_recognition_state"));
        const bool localFilesRecreated = legacyLocalFilesRenamed && query.exec(QStringLiteral(
            "CREATE TABLE local_files ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "root_path TEXT NOT NULL,"
            "relative_path TEXT NOT NULL,"
            "normalized_relative_path TEXT NOT NULL,"
            "file_name TEXT NOT NULL,"
            "extension TEXT NOT NULL,"
            "size_bytes INTEGER NOT NULL CHECK (size_bytes >= 0),"
            "modified_at TEXT NOT NULL,"
            "available INTEGER NOT NULL DEFAULT 1 CHECK (available IN (0, 1)),"
            "last_seen_scan_id INTEGER NOT NULL REFERENCES library_scans(id),"
            "recognition_state TEXT NOT NULL DEFAULT 'unprocessed' "
            "CHECK (recognition_state IN ('unprocessed', 'recognized', 'unrecognized', 'ambiguous', 'associated', 'unsupported')),"
            "extracted_title TEXT NOT NULL DEFAULT '',"
            "media_kind TEXT NOT NULL DEFAULT 'anime',"
            "season INTEGER, episode INTEGER, media_id INTEGER,"
            "recognition_diagnostic TEXT NOT NULL DEFAULT '', recognized_at TEXT,"
            "UNIQUE(root_path, normalized_relative_path))"));
        const bool localFilesCopied = localFilesRecreated && query.exec(QStringLiteral(
            "INSERT INTO local_files (id, root_path, relative_path, normalized_relative_path, file_name, extension, "
            "size_bytes, modified_at, available, last_seen_scan_id, recognition_state, extracted_title, media_kind, "
            "season, episode, media_id, recognition_diagnostic, recognized_at) "
            "SELECT id, root_path, relative_path, normalized_relative_path, file_name, extension, size_bytes, modified_at, "
            "available, last_seen_scan_id, recognition_state, extracted_title, media_kind, season, episode, media_id, "
            "recognition_diagnostic, recognized_at FROM local_files_legacy_recognition_state"));
        recognitionStateConstraintReady = localFilesCopied && query.exec(QStringLiteral(
            "DROP TABLE local_files_legacy_recognition_state"));
    }
    const bool recognitionStateVersionInserted = recognitionStateConstraintReady && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (17)"));
    bool automaticRecognitionColumnExists = false;
    if (recognitionStateVersionInserted && query.exec(QStringLiteral("PRAGMA table_info(user_preferences)"))) {
        while (query.next()) automaticRecognitionColumnExists |= query.value(1).toString() == QStringLiteral("automatic_local_file_recognition");
    }
    const bool automaticRecognitionReady = recognitionStateVersionInserted && (automaticRecognitionColumnExists || query.exec(QStringLiteral(
        "ALTER TABLE user_preferences ADD COLUMN automatic_local_file_recognition INTEGER NOT NULL DEFAULT 1 CHECK (automatic_local_file_recognition IN (0, 1))")));
    const bool automaticRecognitionVersionInserted = automaticRecognitionReady && query.exec(QStringLiteral(
        "INSERT OR IGNORE INTO schema_version (version) VALUES (18)"));
    const bool versionQueried = automaticRecognitionVersionInserted && query.exec(QStringLiteral(
        "SELECT EXISTS(SELECT 1 FROM schema_version WHERE version = 1), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 2), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 3), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 4), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 5), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 6), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 7), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 8), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 9), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 10), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 11), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 12), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 13), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 14), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 15), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 16), "
        "EXISTS(SELECT 1 FROM schema_version WHERE version = 17), EXISTS(SELECT 1 FROM schema_version WHERE version = 18)"));
    const bool versionRecorded = versionQueried && query.next() && query.value(0).toBool()
        && query.value(1).toBool() && query.value(2).toBool() && query.value(3).toBool()
        && query.value(4).toBool() && query.value(5).toBool() && query.value(6).toBool()
        && query.value(7).toBool() && query.value(8).toBool() && query.value(9).toBool()
        && query.value(10).toBool() && query.value(11).toBool() && query.value(12).toBool()
        && query.value(13).toBool() && query.value(14).toBool() && query.value(15).toBool()
        && query.value(16).toBool() && query.value(17).toBool();

    if (versionRecorded) {
        const bool committed = database_.commit();
        if (!committed) {
            lastError_ = database_.lastError().text();
            database_.rollback();
        }
        if (logger_) {
            committed ? logger_->info(LogCategory::Migration, QStringLiteral("SQLite migration completed."))
                      : logger_->error(LogCategory::Migration, database_.lastError().text());
        }
        return committed;
    }

    lastError_ = query.lastError().text();
    if (lastError_.isEmpty()) {
        lastError_ = QStringLiteral("SQLite migration versions 1 through 15 were not recorded.");
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
