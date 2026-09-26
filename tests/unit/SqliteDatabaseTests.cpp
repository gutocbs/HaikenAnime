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
    void migrationFailureExposesDiagnostic();
    void migrationRequiresVersionToBeRecorded();
    void migrationCreatesCoverCacheVersionTwo();
    void migrationCreatesSourceRemovalVersionThree();
    void migrationUpgradesVersionTwoWithoutDataLoss();
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
        QStringLiteral("synopsis"), QStringLiteral("type"), QStringLiteral("status"),
        QStringLiteral("user_list_status"),
        QStringLiteral("source_removed_at")
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
    QCOMPARE(query.value(0).toInt(), 4);
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
    QCOMPARE(values, QList<int>({1, 2, 3, 4}));
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

QTEST_MAIN(SqliteDatabaseTests)
#include "SqliteDatabaseTests.moc"
