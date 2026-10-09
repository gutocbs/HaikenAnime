#include <QCoreApplication>
#include <QFileInfo>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/database/SqliteDatabase.h"

class SqliteDatabaseTests : public QObject {
    Q_OBJECT

private slots:
    void opensAndCreatesDatabaseFile();
    void migrationCreatesMediaSchema();
    void migrationIsIdempotent();
    void migrationCreatesPendingChangesTable();
    void pendingChangesTableStoresVersionColumns();
    void enablesForeignKeyEnforcement();
    void configuresConcurrentAccessPragmas();
    void migrationFailureExposesDiagnostic();
    void migrationRequiresVersionToBeRecorded();
    void migrationCreatesCoverCacheVersionTwo();
    void migrationCreatesSourceRemovalVersionThree();
    void migrationUpgradesVersionTwoWithoutDataLoss();
    void migrationCreatesUserPreferencesVersionFive();
    void migrationCreatesLibraryInventory();
    void migrationUpgradesVersionSixWithoutDataLoss();
    void migrationRequiresVersionSevenAndRollsBack();
    void migrationUpgradesLegacyScannerPreferences();
    void migrationRequiresVersionEightAndRollsBack();
    void migrationAddsHomeSortKeyWithoutLosingExistingData();
    void migrationRequiresVersionNineAndRollsBack();
    void migrationAddsCardStatusPresentationWithoutLosingExistingData();
    void migrationAddsLanguageWithoutLosingExistingData();
    void migrationAddsPreferredTitleWithoutLosingExistingData();
    void migrationAddsExtendedMediaMetadataWithoutLosingExistingData();
    void migrationAddsAdultContentWithDisabledFallbackWithoutLosingExistingData();
    void migrationUpgradesRecognitionStateConstraintWithoutLosingExistingData();
};

void SqliteDatabaseTests::opensAndCreatesDatabaseFile() {
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const auto databasePath = temporaryDirectory.filePath(QStringLiteral("library.sqlite"));
    SqliteDatabase database(databasePath);

    QVERIFY(database.open());
    QVERIFY(database.isOpen());
    QVERIFY(QFileInfo::exists(databasePath));
    QCOMPARE(database.databasePath(), databasePath);
}

void SqliteDatabaseTests::migrationCreatesMediaSchema() {
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());

    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral(
        "SELECT name FROM sqlite_master WHERE type = 'table' AND name IN ('media', 'schema_version')")));

    QStringList tables;
    while (query.next()) {
        tables.append(query.value(0).toString());
    }

    QVERIFY(tables.contains(QStringLiteral("media")));
    QVERIFY(tables.contains(QStringLiteral("schema_version")));

    QVERIFY(query.exec(QStringLiteral("PRAGMA table_info(media)")));
    QStringList columns;
    while (query.next()) {
        columns.append(query.value(1).toString());
    }

    const QStringList expectedColumns{
        QStringLiteral("id"), QStringLiteral("name"), QStringLiteral("english_name"),
        QStringLiteral("original_name"), QStringLiteral("alternative_names"),
        QStringLiteral("total_chapters"), QStringLiteral("consumed_chapters"),
        QStringLiteral("next_chapter"), QStringLiteral("average_score"),
        QStringLiteral("personal_score"), QStringLiteral("cover_url"),
        QStringLiteral("cover_medium_url"), QStringLiteral("cover_large_url"),
        QStringLiteral("cover_extra_large_url"),
        QStringLiteral("synopsis"), QStringLiteral("type"), QStringLiteral("status"),
        QStringLiteral("user_list_status"),
        QStringLiteral("local_path"),
        QStringLiteral("source_removed_at"), QStringLiteral("season"),
        QStringLiteral("season_year"), QStringLiteral("next_airing_episode"),
        QStringLiteral("next_airing_at"), QStringLiteral("anilist_url"),
        QStringLiteral("external_links")
    };

    QCOMPARE(columns, expectedColumns);
}

void SqliteDatabaseTests::migrationIsIdempotent() {
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    QVERIFY(database.migrate());

    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM schema_version")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 19);
}

void SqliteDatabaseTests::migrationCreatesPendingChangesTable() {
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());

    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral(
        "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'anilist_pending_changes'")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("anilist_pending_changes"));
}

void SqliteDatabaseTests::pendingChangesTableStoresVersionColumns() {
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());

    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral("PRAGMA table_info(anilist_pending_changes)")));
    QStringList columns;
    while (query.next()) {
        columns.append(query.value(1).toString());
    }

    QVERIFY(columns.contains(QStringLiteral("local_updated_at")));
    QVERIFY(columns.contains(QStringLiteral("remote_observed_at")));
    QVERIFY(columns.contains(QStringLiteral("remote_version")));
}

void SqliteDatabaseTests::enablesForeignKeyEnforcement() {
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());

    QSqlQuery pragma(database.connection());
    QVERIFY(pragma.exec(QStringLiteral("PRAGMA foreign_keys")));
    QVERIFY(pragma.next());
    QCOMPARE(pragma.value(0).toInt(), 1);

    QSqlQuery insert(database.connection());
    QVERIFY(!insert.exec(QStringLiteral(
        "INSERT INTO anilist_pending_changes "
        "(media_id, field, previous_value, new_value, created_at, local_updated_at, status) "
        "VALUES (999, 5, '{}', '{}', '2026-09-24T00:00:00Z', "
        "'2026-09-24T00:00:00Z', 0)")));
}

void SqliteDatabaseTests::configuresConcurrentAccessPragmas() {
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());

    QSqlQuery pragma(database.connection());
    QVERIFY(pragma.exec(QStringLiteral("PRAGMA busy_timeout")));
    QVERIFY(pragma.next());
    QCOMPARE(pragma.value(0).toInt(), 10000);

    QVERIFY(pragma.exec(QStringLiteral("PRAGMA journal_mode")));
    QVERIFY(pragma.next());
    QCOMPARE(pragma.value(0).toString().toLower(), QStringLiteral("wal"));
}

void SqliteDatabaseTests::migrationFailureExposesDiagnostic() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());

    QSqlQuery incompatibleSchema(database.connection());
    QVERIFY(incompatibleSchema.exec(QStringLiteral(
        "CREATE VIEW schema_version AS SELECT 1 AS version")));

    QVERIFY(!database.migrate());
    QVERIFY(!database.lastError().isEmpty());
}

void SqliteDatabaseTests::migrationRequiresVersionToBeRecorded() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());

    QSqlQuery incompatibleSchema(database.connection());
    QVERIFY(incompatibleSchema.exec(QStringLiteral(
        "CREATE TABLE schema_version (version INTEGER PRIMARY KEY, required_value TEXT NOT NULL)")));

    QVERIFY(!database.migrate());
    QVERIFY(!database.lastError().isEmpty());
}

void SqliteDatabaseTests::migrationCreatesCoverCacheVersionTwo() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());

    QSqlQuery table(database.connection());
    QVERIFY(table.exec(QStringLiteral(
        "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'cover_cache'")));
    QVERIFY(table.next());

    QSqlQuery versions(database.connection());
    QVERIFY(versions.exec(QStringLiteral("SELECT version FROM schema_version ORDER BY version")));
    QList<int> values;
    while (versions.next()) values.append(versions.value(0).toInt());
    QCOMPARE(values, QList<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19}));
}

void SqliteDatabaseTests::migrationCreatesUserPreferencesVersionFive() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));

    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO user_preferences (id, score_minimum, score_maximum, score_step, cover_quality, synchronization_enabled, synchronization_interval_ms) VALUES (1, 0, 10, 1, 'medium', 1, 3600000)")));
    QVERIFY(!query.exec(QStringLiteral(
        "INSERT INTO user_preferences (id, score_minimum, score_maximum, score_step, cover_quality, synchronization_enabled, synchronization_interval_ms) VALUES (2, 0, 10, 1, 'medium', 1, 3600000)")));
    QVERIFY(database.migrate());
    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM schema_version WHERE version = 5")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
}

void SqliteDatabaseTests::migrationCreatesSourceRemovalVersionThree() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());

    QSqlQuery columns(database.connection());
    QVERIFY(columns.exec(QStringLiteral("PRAGMA table_info(media)")));
    bool found = false;
    while (columns.next()) {
        if (columns.value(1).toString() == QStringLiteral("source_removed_at")) {
            found = true;
            QCOMPARE(columns.value(3).toInt(), 0);
        }
    }
    QVERIFY(found);
}

void SqliteDatabaseTests::migrationUpgradesVersionTwoWithoutDataLoss() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QSqlQuery setup(database.connection());
    QVERIFY(setup.exec(QStringLiteral("CREATE TABLE schema_version (version INTEGER PRIMARY KEY)")));
    QVERIFY(setup.exec(QStringLiteral(
        "CREATE TABLE media (id INTEGER PRIMARY KEY, name TEXT NOT NULL, type INTEGER NOT NULL, "
        "status INTEGER NOT NULL)")));
    QVERIFY(setup.exec(QStringLiteral(
        "CREATE TABLE cover_cache (media_id INTEGER PRIMARY KEY, remote_url TEXT NOT NULL, "
        "quality TEXT NOT NULL, relative_path TEXT NOT NULL, mime_type TEXT NOT NULL, "
        "byte_size INTEGER NOT NULL, etag TEXT NOT NULL DEFAULT '', "
        "last_modified TEXT NOT NULL DEFAULT '', validated_at TEXT NOT NULL, "
        "FOREIGN KEY(media_id) REFERENCES media(id) ON DELETE CASCADE)")));
    QVERIFY(setup.exec(QStringLiteral("INSERT INTO schema_version VALUES (1), (2)")));
    QVERIFY(setup.exec(QStringLiteral("INSERT INTO media VALUES (7, 'Preserved', 1, 1)")));
    QVERIFY(setup.exec(QStringLiteral(
        "INSERT INTO cover_cache VALUES (7, 'https://example.test/7.jpg', 'medium', "
        "'covers/7.jpg', 'image/jpeg', 12, '', '', '2026-09-25T00:00:00Z')")));

    QVERIFY2(database.migrate(), qPrintable(database.lastError()));

    QSqlQuery media(database.connection());
    QVERIFY(media.exec(QStringLiteral(
        "SELECT name, source_removed_at, user_list_status FROM media WHERE id = 7")));
    QVERIFY(media.next());
    QCOMPARE(media.value(0).toString(), QStringLiteral("Preserved"));
    QVERIFY(media.value(1).isNull());
    QCOMPARE(media.value(2).toInt(), -1);
    QSqlQuery columns(database.connection());
    QVERIFY(columns.exec(QStringLiteral("PRAGMA table_info(media)")));
    bool foundUserListStatus = false;
    while (columns.next()) {
        if (columns.value(1).toString() != QStringLiteral("user_list_status")) continue;
        foundUserListStatus = true;
        QCOMPARE(columns.value(3).toInt(), 1);
        QCOMPARE(columns.value(4).toString(), QStringLiteral("-1"));
    }
    QVERIFY(foundUserListStatus);
    QSqlQuery cover(database.connection());
    QVERIFY(cover.exec(QStringLiteral("SELECT COUNT(*) FROM cover_cache WHERE media_id = 7")));
    QVERIFY(cover.next());
    QCOMPARE(cover.value(0).toInt(), 1);
}

void SqliteDatabaseTests::migrationCreatesLibraryInventory() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO library_scans (root_path, started_at, status) VALUES ('Q:/', '2026-09-26T00:00:00Z', 'running')")));
    for (const auto &status : {"succeeded", "failed", "interrupted"}) {
        query.prepare(QStringLiteral("UPDATE library_scans SET status = :status"));
        query.bindValue(QStringLiteral(":status"), QString::fromLatin1(status));
        QVERIFY(query.exec());
    }
    QVERIFY(!query.exec(QStringLiteral("UPDATE library_scans SET status = 'invalid'")));
    const auto insert = QStringLiteral(
        "INSERT INTO local_files (root_path, relative_path, normalized_relative_path, file_name, extension, "
        "size_bytes, modified_at, last_seen_scan_id) "
        "VALUES ('Q:/', 'Show/Episode.MKV', 'show/episode.mkv', 'Episode.MKV', '.mkv', 100, "
        "'2026-09-26T00:00:00Z', 1)");
    QVERIFY(query.exec(insert));
    QVERIFY(!query.exec(insert));
    QVERIFY(query.exec(QStringLiteral("SELECT available, recognition_state FROM local_files")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
    QCOMPARE(query.value(1).toString(), QStringLiteral("unprocessed"));
    query.finish();
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO local_files (root_path, relative_path, normalized_relative_path, file_name, extension, "
        "size_bytes, modified_at, last_seen_scan_id) "
        "SELECT 'R:/', relative_path, normalized_relative_path, file_name, extension, size_bytes, "
        "modified_at, last_seen_scan_id FROM local_files")));
    QVERIFY(!query.exec(QStringLiteral("UPDATE local_files SET recognition_state = 'invalid'")));
}

void SqliteDatabaseTests::migrationUpgradesVersionSixWithoutDataLoss() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral("CREATE TABLE schema_version (version INTEGER PRIMARY KEY)")));
    QVERIFY(query.exec(QStringLiteral("INSERT INTO schema_version VALUES (1),(2),(3),(4),(5),(6)")));
    QVERIFY(query.exec(QStringLiteral("CREATE TABLE media (id INTEGER PRIMARY KEY, name TEXT, type INTEGER, status INTEGER)")));
    QVERIFY(query.exec(QStringLiteral("INSERT INTO media VALUES (42, 'Preserved media', 1, 1)")));
    QVERIFY(query.exec(QStringLiteral(
        "CREATE TABLE user_preferences (id INTEGER PRIMARY KEY, score_minimum REAL, score_maximum REAL, "
        "score_step REAL, cover_quality TEXT, synchronization_enabled INTEGER, synchronization_interval_ms INTEGER)")));
    QVERIFY(query.exec(QStringLiteral("INSERT INTO user_preferences VALUES (1, 1, 100, 5, 'large', 0, 12345)")));
    QVERIFY(query.exec(QStringLiteral(
        "CREATE TABLE cover_cache (media_id INTEGER PRIMARY KEY REFERENCES media(id), remote_url TEXT, "
        "quality TEXT, relative_path TEXT, mime_type TEXT, byte_size INTEGER, etag TEXT, last_modified TEXT, validated_at TEXT)")));
    QVERIFY(query.exec(QStringLiteral("INSERT INTO cover_cache VALUES (42, 'https://example.test/42.jpg', 'large', '42.jpg', 'image/jpeg', 12, 'etag', '', '2026-09-26T00:00:00Z')")));
    QVERIFY(query.exec(QStringLiteral(
        "CREATE TABLE anilist_pending_changes (id INTEGER PRIMARY KEY, media_id INTEGER REFERENCES media(id), "
        "field INTEGER, previous_value TEXT, new_value TEXT, created_at TEXT, local_updated_at TEXT, "
        "remote_observed_at TEXT, remote_version TEXT, attempts INTEGER, status INTEGER, last_error TEXT)")));
    QVERIFY(query.exec(QStringLiteral("INSERT INTO anilist_pending_changes VALUES (7, 42, 1, 'old', 'new', '2026-09-26T00:00:00Z', '2026-09-26T00:00:00Z', NULL, 'v1', 2, 0, 'retry')")));
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QVERIFY(query.exec(QStringLiteral("SELECT name FROM media WHERE id = 42")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("Preserved media"));
    QVERIFY(query.exec(QStringLiteral("SELECT score_step, cover_quality, synchronization_interval_ms FROM user_preferences")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 5);
    QCOMPARE(query.value(1).toString(), QStringLiteral("large"));
    QCOMPARE(query.value(2).toInt(), 12345);
    QVERIFY(query.exec(QStringLiteral("SELECT remote_url, etag FROM cover_cache WHERE media_id = 42")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("https://example.test/42.jpg"));
    QCOMPARE(query.value(1).toString(), QStringLiteral("etag"));
    QVERIFY(query.exec(QStringLiteral("SELECT new_value, attempts, last_error FROM anilist_pending_changes WHERE id = 7")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("new"));
    QCOMPARE(query.value(1).toInt(), 2);
    QCOMPARE(query.value(2).toString(), QStringLiteral("retry"));
    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM schema_version WHERE version = 7")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
}

void SqliteDatabaseTests::migrationRequiresVersionSevenAndRollsBack() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral("CREATE TABLE schema_version (version INTEGER PRIMARY KEY)")));
    QVERIFY(query.exec(QStringLiteral("CREATE TRIGGER reject_seven BEFORE INSERT ON schema_version "
                                      "WHEN NEW.version = 7 BEGIN SELECT RAISE(IGNORE); END")));
    QVERIFY(!database.migrate());
    QVERIFY(!database.lastError().isEmpty());
    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM sqlite_master WHERE name IN ('local_files', 'library_scans', 'media')")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 0);
}

void SqliteDatabaseTests::migrationUpgradesLegacyScannerPreferences() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath("legacy.sqlite"));
    QVERIFY(database.open());
    QSqlQuery query(database.connection());
    QVERIFY(query.exec("CREATE TABLE user_preferences (id INTEGER PRIMARY KEY, score_minimum REAL, score_maximum REAL, score_step REAL, cover_quality TEXT, synchronization_enabled INTEGER, synchronization_interval_ms INTEGER)"));
    QVERIFY(query.exec("INSERT INTO user_preferences VALUES (1, 0, 100, 5, 'large', 0, 1800000)"));
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QVERIFY(database.migrate());
    QVERIFY(query.exec("SELECT library_root, scan_extensions, score_step FROM user_preferences"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("Q:\\"));
    QCOMPARE(query.value(1).toString(), QStringLiteral("[\".mkv\",\".mp4\",\".avi\",\".webm\",\".m4v\",\".mov\",\".wmv\",\".ts\"]"));
    QCOMPARE(query.value(2).toInt(), 5);
    QVERIFY(query.exec("SELECT COUNT(*) FROM schema_version WHERE version = 8"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
}

void SqliteDatabaseTests::migrationRequiresVersionEightAndRollsBack() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath("legacy.sqlite"));
    QVERIFY(database.open());
    QSqlQuery query(database.connection());
    QVERIFY(query.exec("CREATE TABLE schema_version (version INTEGER PRIMARY KEY)"));
    QVERIFY(query.exec("CREATE TRIGGER reject_eight BEFORE INSERT ON schema_version WHEN NEW.version = 8 BEGIN SELECT RAISE(IGNORE); END"));
    QVERIFY(!database.migrate());
    QVERIFY(!database.lastError().isEmpty());
    QVERIFY(query.exec("SELECT COUNT(*) FROM sqlite_master WHERE name = 'user_preferences'"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 0);
}

void SqliteDatabaseTests::migrationAddsHomeSortKeyWithoutLosingExistingData() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath("legacy.sqlite"));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QSqlQuery query(database.connection());
    QVERIFY(query.exec("INSERT INTO media (id, name, type, status) VALUES (17, 'Preserved media', 1, 1)"));
    QVERIFY(query.exec("INSERT INTO anilist_pending_changes (media_id, field, previous_value, new_value, created_at, local_updated_at, status) VALUES (17, 1, 'old', 'new', '2026-09-27T00:00:00Z', '2026-09-27T00:00:00Z', 0)"));
    QVERIFY(query.exec("INSERT INTO cover_cache (media_id, remote_url, quality, relative_path, mime_type, byte_size, validated_at) VALUES (17, 'https://example.test/17.jpg', 'medium', 'covers/17.jpg', 'image/jpeg', 17, '2026-09-27T00:00:00Z')"));
    QVERIFY(query.exec("INSERT INTO library_scans (root_path, started_at, status) VALUES ('R:/', '2026-09-27T00:00:00Z', 'succeeded')"));
    QVERIFY(query.exec("INSERT INTO local_files (root_path, relative_path, normalized_relative_path, file_name, extension, size_bytes, modified_at, last_seen_scan_id) VALUES ('R:/', 'show/episode.mkv', 'show/episode.mkv', 'episode.mkv', '.mkv', 123, '2026-09-27T00:00:00Z', 1)"));
    QVERIFY(query.exec("DROP TABLE user_preferences"));
    QVERIFY(query.exec("CREATE TABLE user_preferences (id INTEGER PRIMARY KEY, score_minimum REAL, score_maximum REAL, score_step REAL, cover_quality TEXT, synchronization_enabled INTEGER, synchronization_interval_ms INTEGER, library_root TEXT, scan_extensions TEXT)"));
    QVERIFY(query.exec("INSERT INTO user_preferences (id, score_minimum, score_maximum, score_step, cover_quality, synchronization_enabled, synchronization_interval_ms, library_root, scan_extensions) VALUES (1, 0, 10, 1, 'medium', 1, 3600000, 'R:/', '[\".mkv\"]')"));
    QVERIFY(query.exec("DELETE FROM schema_version WHERE version = 9"));

    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QVERIFY(query.exec("SELECT home_sort_key FROM user_preferences WHERE id = 1"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("title_asc"));
    QVERIFY(query.exec("SELECT COUNT(*) FROM media")); QVERIFY(query.next()); QCOMPARE(query.value(0).toInt(), 1);
    QVERIFY(query.exec("SELECT COUNT(*) FROM anilist_pending_changes")); QVERIFY(query.next()); QCOMPARE(query.value(0).toInt(), 1);
    QVERIFY(query.exec("SELECT COUNT(*) FROM cover_cache")); QVERIFY(query.next()); QCOMPARE(query.value(0).toInt(), 1);
    QVERIFY(query.exec("SELECT COUNT(*) FROM local_files")); QVERIFY(query.next()); QCOMPARE(query.value(0).toInt(), 1);
    QVERIFY(query.exec("SELECT COUNT(*) FROM schema_version WHERE version = 9")); QVERIFY(query.next()); QCOMPARE(query.value(0).toInt(), 1);
}

void SqliteDatabaseTests::migrationRequiresVersionNineAndRollsBack() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath("legacy.sqlite"));
    QVERIFY(database.open());
    QSqlQuery query(database.connection());
    QVERIFY(query.exec("CREATE TABLE schema_version (version INTEGER PRIMARY KEY)"));
    QVERIFY(query.exec("CREATE TRIGGER reject_nine BEFORE INSERT ON schema_version WHEN NEW.version = 9 BEGIN SELECT RAISE(IGNORE); END"));
    QVERIFY(!database.migrate());
    QVERIFY(!database.lastError().isEmpty());
    QVERIFY(query.exec("SELECT COUNT(*) FROM sqlite_master WHERE name = 'user_preferences'"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 0);
}

void SqliteDatabaseTests::migrationAddsCardStatusPresentationWithoutLosingExistingData() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("legacy.sqlite")));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral("INSERT INTO media (id, name, type, status) VALUES (17, 'Preserved media', 1, 1)")));
    QVERIFY(query.exec(QStringLiteral("DROP TABLE user_preferences")));
    QVERIFY(query.exec(QStringLiteral(
        "CREATE TABLE user_preferences (id INTEGER PRIMARY KEY, score_minimum REAL, score_maximum REAL, "
        "score_step REAL, cover_quality TEXT, synchronization_enabled INTEGER, synchronization_interval_ms INTEGER, "
        "home_sort_key TEXT, library_root TEXT, scan_extensions TEXT)")));
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO user_preferences VALUES (1, 0, 10, 1, 'medium', 1, 3600000, 'title_desc', 'R:/', '[\".mkv\"]')")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM schema_version WHERE version = 10")));

    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QVERIFY(query.exec(QStringLiteral("SELECT home_sort_key, card_status_presentation FROM user_preferences WHERE id = 1")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("title_desc"));
    QCOMPARE(query.value(1).toString(), QStringLiteral("personal-list-status"));
    QVERIFY(query.exec(QStringLiteral("SELECT name FROM media WHERE id = 17")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("Preserved media"));
}

void SqliteDatabaseTests::migrationAddsLanguageWithoutLosingExistingData() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("legacy.sqlite")));
    QVERIFY(database.open());
    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral("CREATE TABLE schema_version (version INTEGER PRIMARY KEY)")));
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO schema_version (version) VALUES (1), (2), (3), (4), (5), (6), (7), (8), (9), (10)")));
    QVERIFY(query.exec(QStringLiteral(
        "CREATE TABLE user_preferences ("
        "id INTEGER PRIMARY KEY, score_minimum REAL, score_maximum REAL, score_step REAL, "
        "cover_quality TEXT, synchronization_enabled INTEGER, synchronization_interval_ms INTEGER, "
        "home_sort_key TEXT, card_status_presentation TEXT, library_root TEXT, scan_extensions TEXT)")));
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO user_preferences VALUES "
        "(1, 0, 10, 1, 'large', 0, 1800000, 'title_desc', 'media-release-status', "
        "'R:/Anime', '[\".mkv\"]')")));

    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QVERIFY(query.exec(QStringLiteral("SELECT language_key, cover_quality, synchronization_enabled, home_sort_key FROM user_preferences WHERE id = 1")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("pt-BR"));
    QCOMPARE(query.value(1).toString(), QStringLiteral("large"));
    QCOMPARE(query.value(2).toBool(), false);
    QCOMPARE(query.value(3).toString(), QStringLiteral("title_desc"));
}

void SqliteDatabaseTests::migrationAddsPreferredTitleWithoutLosingExistingData() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("legacy.sqlite")));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral("INSERT INTO media (id, name, type, status) VALUES (17, 'Preserved media', 1, 1)")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM schema_version WHERE version = 12")));
    QVERIFY(query.exec(QStringLiteral("ALTER TABLE user_preferences RENAME TO user_preferences_newer")));
    QVERIFY(query.exec(QStringLiteral(
        "CREATE TABLE user_preferences (id INTEGER PRIMARY KEY, score_minimum REAL, score_maximum REAL, "
        "score_step REAL, cover_quality TEXT, synchronization_enabled INTEGER, synchronization_interval_ms INTEGER, "
        "home_sort_key TEXT, card_status_presentation TEXT, language_key TEXT, library_root TEXT, scan_extensions TEXT)")));
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO user_preferences VALUES "
        "(1, 0, 10, 1, 'large', 0, 1800000, 'title_desc', 'media-release-status', 'en', "
        "'R:/Anime', '[\".mkv\"]')")));
    QVERIFY(query.exec(QStringLiteral("DROP TABLE user_preferences_newer")));

    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QVERIFY(query.exec(QStringLiteral("SELECT preferred_title_key, language_key, cover_quality FROM user_preferences WHERE id = 1")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("romaji"));
    QCOMPARE(query.value(1).toString(), QStringLiteral("en"));
    QCOMPARE(query.value(2).toString(), QStringLiteral("large"));
    QVERIFY(query.exec(QStringLiteral("SELECT name FROM media WHERE id = 17")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("Preserved media"));
}

void SqliteDatabaseTests::migrationAddsExtendedMediaMetadataWithoutLosingExistingData() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("legacy.sqlite")));
    QVERIFY(database.open());
    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral("CREATE TABLE schema_version (version INTEGER PRIMARY KEY)")));
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO schema_version (version) VALUES "
        "(1),(2),(3),(4),(5),(6),(7),(8),(9),(10),(11),(12)")));
    QVERIFY(query.exec(QStringLiteral(
        "CREATE TABLE media ("
        "id INTEGER PRIMARY KEY, name TEXT NOT NULL, english_name TEXT, original_name TEXT, "
        "alternative_names TEXT NOT NULL DEFAULT '[]', total_chapters INTEGER NOT NULL DEFAULT 0, "
        "consumed_chapters INTEGER NOT NULL DEFAULT 0, next_chapter INTEGER NOT NULL DEFAULT 0, "
        "average_score INTEGER NOT NULL DEFAULT 0, personal_score INTEGER NOT NULL DEFAULT 0, "
        "cover_url TEXT, cover_medium_url TEXT, cover_large_url TEXT, cover_extra_large_url TEXT, "
        "synopsis TEXT, type INTEGER NOT NULL, status INTEGER NOT NULL, "
        "user_list_status INTEGER NOT NULL DEFAULT -1, source_removed_at TEXT)")));
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO media (id, name, consumed_chapters, personal_score, type, status, user_list_status) "
        "VALUES (17, 'Preserved media', 12, 90, 1, 1, 4)")));
    QVERIFY(query.exec(QStringLiteral(
        "CREATE TABLE cover_cache (media_id INTEGER PRIMARY KEY, remote_url TEXT NOT NULL, "
        "quality TEXT NOT NULL, relative_path TEXT NOT NULL, mime_type TEXT NOT NULL, "
        "byte_size INTEGER NOT NULL, etag TEXT NOT NULL DEFAULT '', last_modified TEXT NOT NULL DEFAULT '', "
        "validated_at TEXT NOT NULL, FOREIGN KEY(media_id) REFERENCES media(id) ON DELETE CASCADE)")));
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO cover_cache (media_id, remote_url, quality, relative_path, mime_type, byte_size, validated_at) "
        "VALUES (17, 'https://example.test/17.jpg', 'large', 'covers/17.jpg', 'image/jpeg', 17, "
        "'2026-09-27T00:00:00Z')")));
    QVERIFY(query.exec(QStringLiteral(
        "CREATE TABLE library_scans (id INTEGER PRIMARY KEY AUTOINCREMENT, root_path TEXT NOT NULL, "
        "started_at TEXT NOT NULL, finished_at TEXT, status TEXT NOT NULL, observed_count INTEGER NOT NULL DEFAULT 0, "
        "diagnostic TEXT NOT NULL DEFAULT '')")));
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO library_scans (id, root_path, started_at, status) "
        "VALUES (3, 'R:/Anime', '2026-09-27T00:00:00Z', 'succeeded')")));
    QVERIFY(query.exec(QStringLiteral(
        "CREATE TABLE local_files (id INTEGER PRIMARY KEY AUTOINCREMENT, root_path TEXT NOT NULL, "
        "relative_path TEXT NOT NULL, normalized_relative_path TEXT NOT NULL, file_name TEXT NOT NULL, "
        "extension TEXT NOT NULL, size_bytes INTEGER NOT NULL, modified_at TEXT NOT NULL, "
        "available INTEGER NOT NULL DEFAULT 1, last_seen_scan_id INTEGER NOT NULL REFERENCES library_scans(id), "
        "recognition_state TEXT NOT NULL DEFAULT 'unprocessed', UNIQUE(root_path, normalized_relative_path))")));
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO local_files (root_path, relative_path, normalized_relative_path, file_name, extension, "
        "size_bytes, modified_at, last_seen_scan_id) VALUES "
        "('R:/Anime', 'Show/Episode.mkv', 'show/episode.mkv', 'Episode.mkv', '.mkv', 17, "
        "'2026-09-27T00:00:00Z', 3)")));

    QVERIFY2(database.migrate(), qPrintable(database.lastError()));

    QVERIFY(query.exec(QStringLiteral(
        "SELECT name, consumed_chapters, personal_score, user_list_status, local_path FROM media WHERE id = 17")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("Preserved media"));
    QCOMPARE(query.value(1).toInt(), 12);
    QCOMPARE(query.value(2).toInt(), 90);
    QCOMPARE(query.value(3).toInt(), 4);
    QVERIFY(query.value(4).toString().isEmpty());
    QVERIFY(query.exec(QStringLiteral("SELECT remote_url FROM cover_cache WHERE media_id = 17")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("https://example.test/17.jpg"));
    QVERIFY(query.exec(QStringLiteral("SELECT file_name, available FROM local_files WHERE id = 1")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("Episode.mkv"));
    QCOMPARE(query.value(1).toInt(), 1);
    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM schema_version WHERE version = 13")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM schema_version WHERE version = 15")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
}

void SqliteDatabaseTests::migrationAddsAdultContentWithDisabledFallbackWithoutLosingExistingData() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("legacy.sqlite")));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM schema_version WHERE version = 14")));
    QVERIFY(query.exec(QStringLiteral("ALTER TABLE user_preferences RENAME TO user_preferences_newer")));
    QVERIFY(query.exec(QStringLiteral(
        "CREATE TABLE user_preferences (id INTEGER PRIMARY KEY, score_minimum REAL, score_maximum REAL, "
        "score_step REAL, cover_quality TEXT, synchronization_enabled INTEGER, synchronization_interval_ms INTEGER, "
        "home_sort_key TEXT, card_status_presentation TEXT, language_key TEXT, preferred_title_key TEXT, "
        "library_root TEXT, scan_extensions TEXT)")));
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO user_preferences VALUES "
        "(1, 0, 10, 1, 'large', 0, 1800000, 'title_desc', 'media-release-status', 'en', 'native', "
        "'R:/Anime', '[\".mkv\"]')")));
    QVERIFY(query.exec(QStringLiteral("DROP TABLE user_preferences_newer")));

    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QVERIFY(query.exec(QStringLiteral(
        "SELECT include_adult_content, home_sort_key, preferred_title_key FROM user_preferences WHERE id = 1")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toBool(), false);
    QCOMPARE(query.value(1).toString(), QStringLiteral("title_desc"));
    QCOMPARE(query.value(2).toString(), QStringLiteral("native"));
}

void SqliteDatabaseTests::migrationUpgradesRecognitionStateConstraintWithoutLosingExistingData() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("legacy.sqlite")));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));

    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO library_scans (root_path, started_at, status) "
        "VALUES ('Q:/Animes', '2026-10-03T00:00:00Z', 'succeeded')")));
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO local_files (root_path, relative_path, normalized_relative_path, file_name, extension, "
        "size_bytes, modified_at, last_seen_scan_id, recognition_state, extracted_title, episode) VALUES "
        "('Q:/Animes', 'Show/02.mkv', 'show/02.mkv', '02.mkv', '.mkv', 123, "
        "'2026-10-03T00:00:00Z', 1, 'associated', 'Preserved', 2)")));
    QVERIFY(query.exec(QStringLiteral("ALTER TABLE local_files RENAME TO local_files_current")));
    QVERIFY(query.exec(QStringLiteral(
        "CREATE TABLE local_files ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, root_path TEXT NOT NULL, relative_path TEXT NOT NULL, "
        "normalized_relative_path TEXT NOT NULL, file_name TEXT NOT NULL, extension TEXT NOT NULL, "
        "size_bytes INTEGER NOT NULL CHECK (size_bytes >= 0), modified_at TEXT NOT NULL, "
        "available INTEGER NOT NULL DEFAULT 1 CHECK (available IN (0, 1)), "
        "last_seen_scan_id INTEGER NOT NULL REFERENCES library_scans(id), "
        "recognition_state TEXT NOT NULL DEFAULT 'unprocessed' "
        "CHECK (recognition_state IN ('unprocessed', 'recognized', 'unrecognized', 'associated')), "
        "extracted_title TEXT NOT NULL DEFAULT '', media_kind TEXT NOT NULL DEFAULT 'anime', "
        "season INTEGER, episode INTEGER, media_id INTEGER, recognition_diagnostic TEXT NOT NULL DEFAULT '', "
        "recognized_at TEXT, UNIQUE(root_path, normalized_relative_path))")));
    QVERIFY(query.exec(QStringLiteral("INSERT INTO local_files SELECT * FROM local_files_current")));
    QVERIFY(query.exec(QStringLiteral("DROP TABLE local_files_current")));

    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QVERIFY(query.exec(QStringLiteral("UPDATE local_files SET recognition_state = 'ambiguous' WHERE id = 1")));
    QVERIFY(query.exec(QStringLiteral("SELECT extracted_title, episode FROM local_files WHERE id = 1")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("Preserved"));
    QCOMPARE(query.value(1).toInt(), 2);
    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM schema_version WHERE version = 17")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
}

QTEST_MAIN(SqliteDatabaseTests)
#include "SqliteDatabaseTests.moc"
