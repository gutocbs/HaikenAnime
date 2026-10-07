#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/database/SqliteCoverCacheRepository.h"
#include "../../src/infrastructure/database/SqliteDatabase.h"

namespace {
const auto ReadSql = QStringLiteral(":/sqlite/queries/read-cover-cache.sql");
const auto UpsertSql = QStringLiteral(":/sqlite/queries/upsert-cover-cache.sql");
const auto DeleteSql = QStringLiteral(":/sqlite/queries/delete-cover-cache.sql");
const auto ClearSql = QStringLiteral(":/sqlite/queries/clear-cover-cache.sql");

void InsertMedia(QSqlDatabase database, const int id) {
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "INSERT INTO media (id, name, type, status) VALUES (:id, 'Media', 1, 1)"));
    query.bindValue(QStringLiteral(":id"), id);
    QVERIFY(query.exec());
}

CoverCacheEntry Entry(const int mediaId, const QString &url, const QString &path) {
    CoverCacheEntry entry;
    entry.mediaId = mediaId;
    entry.remoteUrl = url;
    entry.quality = CoverQuality::Medium;
    entry.relativePath = path;
    entry.mimeType = QStringLiteral("image/jpeg");
    entry.byteSize = 1234;
    entry.etag = QStringLiteral("etag-value");
    entry.lastModified = QStringLiteral("Wed, 25 Sep 2026 12:00:00 GMT");
    entry.validatedAt = QDateTime::fromString(QStringLiteral("2026-09-25T12:00:00Z"), Qt::ISODate);
    return entry;
}
}

class SqliteCoverCacheRepositoryTests final : public QObject {
    Q_OBJECT

private slots:
    void roundTripsAndReplacesEntry();
    void persistsEntryWithoutHttpValidators();
    void rejectsAbsolutePath();
    void cascadesMediaDeletionAndClearsEntries();
    void clearReportsCountAndPreservesPrimaryData();
};

void SqliteCoverCacheRepositoryTests::roundTripsAndReplacesEntry() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    InsertMedia(database.connection(), 42);
    SqliteCoverCacheRepository repository(database.connection(), ReadSql, UpsertSql,
                                          DeleteSql, ClearSql);
    QString error;

    QVERIFY2(repository.Upsert(Entry(42, QStringLiteral("https://img/old.jpg"),
                                     QStringLiteral("covers/42-old.jpg")), error), qPrintable(error));
    auto replacement = Entry(42, QStringLiteral("https://img/new.jpg"),
                             QStringLiteral("covers/42-new.jpg"));
    replacement.quality = CoverQuality::Large;
    QVERIFY2(repository.Upsert(replacement, error), qPrintable(error));

    QHash<int, CoverCacheEntry> entries;
    QVERIFY2(repository.ReadAll(entries, error), qPrintable(error));
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.value(42).remoteUrl, QStringLiteral("https://img/new.jpg"));
    QCOMPARE(entries.value(42).quality, CoverQuality::Large);
    QCOMPARE(entries.value(42).relativePath, QStringLiteral("covers/42-new.jpg"));
    QCOMPARE(entries.value(42).validatedAt.toUTC(), replacement.validatedAt.toUTC());
}

void SqliteCoverCacheRepositoryTests::persistsEntryWithoutHttpValidators() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    InsertMedia(database.connection(), 42);
    SqliteCoverCacheRepository repository(database.connection(), ReadSql, UpsertSql,
                                          DeleteSql, ClearSql);
    auto entry = Entry(42, QStringLiteral("https://img/cover.jpg"),
                       QStringLiteral("42-cover.jpg"));
    entry.etag = {};
    entry.lastModified = {};
    QString error;

    QVERIFY2(repository.Upsert(entry, error), qPrintable(error));
    QHash<int, CoverCacheEntry> entries;
    QVERIFY2(repository.ReadAll(entries, error), qPrintable(error));
    QVERIFY(entries.contains(42));
    QCOMPARE(entries.value(42).etag, QStringLiteral(""));
    QCOMPARE(entries.value(42).lastModified, QStringLiteral(""));
}

void SqliteCoverCacheRepositoryTests::rejectsAbsolutePath() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    InsertMedia(database.connection(), 42);
    SqliteCoverCacheRepository repository(database.connection(), ReadSql, UpsertSql,
                                          DeleteSql, ClearSql);
    QString error;

    QVERIFY(!repository.Upsert(Entry(42, QStringLiteral("https://img/cover.jpg"),
                                     QDir::rootPath() + QStringLiteral("cover.jpg")), error));
    QVERIFY(error.contains(QStringLiteral("relative"), Qt::CaseInsensitive));
}

void SqliteCoverCacheRepositoryTests::cascadesMediaDeletionAndClearsEntries() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    InsertMedia(database.connection(), 42);
    InsertMedia(database.connection(), 43);
    SqliteCoverCacheRepository repository(database.connection(), ReadSql, UpsertSql,
                                          DeleteSql, ClearSql);
    QString error;
    QVERIFY(repository.Upsert(Entry(42, QStringLiteral("https://img/42.jpg"),
                                    QStringLiteral("covers/42.jpg")), error));
    QVERIFY(repository.Upsert(Entry(43, QStringLiteral("https://img/43.jpg"),
                                    QStringLiteral("covers/43.jpg")), error));

    QSqlQuery deleteMedia(database.connection());
    QVERIFY(deleteMedia.exec(QStringLiteral("DELETE FROM media WHERE id = 42")));
    QHash<int, CoverCacheEntry> entries;
    QVERIFY(repository.ReadAll(entries, error));
    QVERIFY(!entries.contains(42));
    QVERIFY(entries.contains(43));

    int removedEntries = 0;
    QVERIFY(repository.Clear(removedEntries, error));
    QCOMPARE(removedEntries, 1);
    QVERIFY(repository.ReadAll(entries, error));
    QVERIFY(entries.isEmpty());
}

void SqliteCoverCacheRepositoryTests::clearReportsCountAndPreservesPrimaryData() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    InsertMedia(database.connection(), 42);
    QSqlQuery setup(database.connection());
    QVERIFY(setup.exec(QStringLiteral("UPDATE media SET consumed_chapters = 7 WHERE id = 42")));
    QVERIFY(setup.exec(QStringLiteral(
        "INSERT INTO anilist_pending_changes "
        "(media_id, field, previous_value, new_value, created_at, local_updated_at, "
        "remote_version, status) VALUES (42, 1, 'old', 'new', '2026-10-07T00:00:00Z', "
        "'2026-10-07T00:00:00Z', '', 0)")));
    QVERIFY(setup.exec(QStringLiteral(
        "INSERT INTO user_preferences "
        "(id, score_minimum, score_maximum, score_step, cover_quality, "
        "synchronization_enabled, synchronization_interval_ms) "
        "VALUES (1, 0, 10, 1, 'medium', 1, 60000)")));
    QVERIFY(setup.exec(QStringLiteral(
        "INSERT INTO library_scans "
        "(id, root_path, started_at, status) "
        "VALUES (1, 'Q:/Library', '2026-10-07T00:00:00Z', 'succeeded')")));
    QVERIFY(setup.exec(QStringLiteral(
        "INSERT INTO local_files "
        "(root_path, relative_path, normalized_relative_path, file_name, extension, "
        "size_bytes, modified_at, last_seen_scan_id) "
        "VALUES ('Q:/Library', 'Show/Episode.mkv', 'show/episode.mkv', 'Episode.mkv', "
        "'.mkv', 123, '2026-10-07T00:00:00Z', 1)")));
    SqliteCoverCacheRepository repository(database.connection(), ReadSql, UpsertSql,
                                          DeleteSql, ClearSql);
    QString error;
    QVERIFY(repository.Upsert(Entry(42, QStringLiteral("https://img/42.jpg"),
                                    QStringLiteral("covers/42.jpg")), error));

    int removedEntries = -1;
    QVERIFY2(repository.Clear(removedEntries, error), qPrintable(error));

    QCOMPARE(removedEntries, 1);
    QVERIFY(setup.exec(QStringLiteral(
        "SELECT consumed_chapters FROM media WHERE id = 42")));
    QVERIFY(setup.next());
    QCOMPARE(setup.value(0).toInt(), 7);
    const QStringList preservedTables = {
        QStringLiteral("anilist_pending_changes"),
        QStringLiteral("user_preferences"),
        QStringLiteral("library_scans"),
        QStringLiteral("local_files")
    };
    for (const auto &table : preservedTables) {
        QVERIFY2(setup.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(table)),
                 qPrintable(setup.lastError().text()));
        QVERIFY(setup.next());
        QCOMPARE(setup.value(0).toInt(), 1);
    }
}

QTEST_MAIN(SqliteCoverCacheRepositoryTests)
#include "SqliteCoverCacheRepositoryTests.moc"
