#include <QDir>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/database/SqliteCoverCacheRepository.h"
#include "../../src/infrastructure/database/SqliteDatabase.h"

namespace {
const auto ReadSql = QStringLiteral(
    "SELECT media_id, remote_url, quality, relative_path, mime_type, byte_size, "
    "etag, last_modified, validated_at FROM cover_cache");
const auto UpsertSql = QStringLiteral(
    "INSERT INTO cover_cache (media_id, remote_url, quality, relative_path, mime_type, "
    "byte_size, etag, last_modified, validated_at) VALUES (:media_id, :remote_url, "
    ":quality, :relative_path, :mime_type, :byte_size, :etag, :last_modified, "
    ":validated_at) ON CONFLICT(media_id) DO UPDATE SET remote_url=excluded.remote_url, "
    "quality=excluded.quality, relative_path=excluded.relative_path, "
    "mime_type=excluded.mime_type, byte_size=excluded.byte_size, etag=excluded.etag, "
    "last_modified=excluded.last_modified, validated_at=excluded.validated_at");
const auto DeleteSql = QStringLiteral("DELETE FROM cover_cache WHERE media_id = :media_id");
const auto ClearSql = QStringLiteral("DELETE FROM cover_cache");

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

    QVERIFY(repository.Clear(error));
    QVERIFY(repository.ReadAll(entries, error));
    QVERIFY(entries.isEmpty());
}

QTEST_MAIN(SqliteCoverCacheRepositoryTests)
#include "SqliteCoverCacheRepositoryTests.moc"
