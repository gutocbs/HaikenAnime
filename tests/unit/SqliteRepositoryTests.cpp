#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/database/SqliteDatabase.h"
#include "../../src/infrastructure/database/SqliteMediaRepository.h"
#include "../../src/infrastructure/database/SqlitePendingChangeRepository.h"

namespace {
const auto UpsertMedia = QStringLiteral(
    "INSERT INTO media (id, name, english_name, original_name, alternative_names, "
    "total_chapters, average_score, cover_url, synopsis, type, status) "
    "VALUES (:id, :name, :english_name, :original_name, :alternative_names, "
    ":total_chapters, :average_score, :cover_url, :synopsis, :type, :status) "
    "ON CONFLICT(id) DO UPDATE SET name = excluded.name");

const auto ReadMedia = QStringLiteral(
    "SELECT id, name, english_name, original_name, alternative_names, total_chapters, "
    "consumed_chapters, next_chapter, average_score, personal_score, cover_url, synopsis, "
    "type, status FROM media ORDER BY id");

const auto EnqueueChange = QStringLiteral(
    "INSERT INTO anilist_pending_changes "
    "(media_id, field, previous_value, new_value, created_at, local_updated_at, "
    "remote_observed_at, remote_version, attempts, status, last_error) VALUES "
    "(:media_id, :field, :previous_value, :new_value, :created_at, :local_updated_at, "
    ":remote_observed_at, :remote_version, :attempts, :status, :last_error)");

const auto ReadChanges = QStringLiteral(
    "SELECT id, media_id, field, previous_value, new_value, created_at, local_updated_at, "
    "remote_observed_at, remote_version, attempts, status, last_error "
    "FROM anilist_pending_changes WHERE media_id = :media_id AND status IN (0, 1, 3, 4) "
    "ORDER BY id");

const auto UpdateChange = QStringLiteral(
    "UPDATE anilist_pending_changes SET status = :status, attempts = :attempts, "
    "last_error = :last_error WHERE id = :id AND status IN (0, 1, 3, 4)");

bool insertMedia(QSqlDatabase database, int mediaId) {
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "INSERT INTO media (id, name, alternative_names, type, status) "
        "VALUES (:id, 'Media', '[]', 0, 0)"));
    query.bindValue(QStringLiteral(":id"), mediaId);
    return query.exec();
}
}

class SqliteRepositoryTests : public QObject {
    Q_OBJECT

private slots:
    void mediaRoundTripPreservesAlternativeNames();
    void pendingChangeRoundTripPreservesRemoteObservedAt();
    void malformedPendingValueFailsInsteadOfBecomingEmptyText();
    void fractionalPendingIntegerIsRejected();
    void updatingMissingPendingChangeFails();
};

void SqliteRepositoryTests::mediaRoundTripPreservesAlternativeNames() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    SqliteMediaRepository repository(database.connection(), UpsertMedia, ReadMedia);

    Media expected;
    expected.Id = 7;
    expected.Name = QStringLiteral("Primary");
    expected.AlternativeNames = {QStringLiteral("Alias A"), QStringLiteral("Alias B")};
    QString error;
    QVERIFY(repository.upsert({expected}, error));

    QList<Media> actual;
    QVERIFY(repository.readAll(actual, error));
    QCOMPARE(actual.size(), 1);
    QCOMPARE(actual.first().AlternativeNames, expected.AlternativeNames);
}

void SqliteRepositoryTests::pendingChangeRoundTripPreservesRemoteObservedAt() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    QVERIFY(insertMedia(database.connection(), 7));
    SqlitePendingChangeRepository repository(
        database.connection(), EnqueueChange, ReadChanges, UpdateChange);

    AniListPendingChange expected;
    expected.mediaId = 7;
    expected.field = AniListField::Progress;
    expected.previousValue = 1;
    expected.newValue = 2;
    expected.createdAt = QDateTime::fromString(QStringLiteral("2026-09-24T10:00:00Z"), Qt::ISODate);
    expected.localUpdatedAt = expected.createdAt;
    expected.remoteObservedAt = QDateTime::fromString(
        QStringLiteral("2026-09-24T09:30:00Z"), Qt::ISODate);
    QString error;
    QVERIFY2(repository.enqueue(expected, error), qPrintable(error));

    QList<AniListPendingChange> actual;
    QVERIFY(repository.getPending(7, actual, error));
    QCOMPARE(actual.size(), 1);
    QCOMPARE(actual.first().remoteObservedAt, expected.remoteObservedAt);
}

void SqliteRepositoryTests::malformedPendingValueFailsInsteadOfBecomingEmptyText() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    QVERIFY(insertMedia(database.connection(), 7));
    QSqlQuery insert(database.connection());
    QVERIFY(insert.exec(QStringLiteral(
        "INSERT INTO anilist_pending_changes "
        "(media_id, field, previous_value, new_value, created_at, local_updated_at, status) "
        "VALUES (7, 5, 'not-json', '{\"type\":\"int\",\"value\":2}', "
        "'2026-09-24T10:00:00Z', '2026-09-24T10:00:00Z', 0)")));
    SqlitePendingChangeRepository repository(
        database.connection(), EnqueueChange, ReadChanges, UpdateChange);

    QList<AniListPendingChange> changes;
    QString error;
    QVERIFY(!repository.getPending(7, changes, error));
    QVERIFY(!error.isEmpty());
    QVERIFY(changes.isEmpty());
}

void SqliteRepositoryTests::fractionalPendingIntegerIsRejected() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    QVERIFY(insertMedia(database.connection(), 7));
    QSqlQuery insert(database.connection());
    QVERIFY(insert.exec(QStringLiteral(
        "INSERT INTO anilist_pending_changes "
        "(media_id, field, previous_value, new_value, created_at, local_updated_at, status) "
        "VALUES (7, 5, '{\"type\":\"int\",\"value\":1.5}', "
        "'{\"type\":\"int\",\"value\":2}', '2026-09-24T10:00:00Z', "
        "'2026-09-24T10:00:00Z', 0)")));
    SqlitePendingChangeRepository repository(
        database.connection(), EnqueueChange, ReadChanges, UpdateChange);

    QList<AniListPendingChange> changes;
    QString error;
    QVERIFY(!repository.getPending(7, changes, error));
    QVERIFY(!error.isEmpty());
}

void SqliteRepositoryTests::updatingMissingPendingChangeFails() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    SqlitePendingChangeRepository repository(
        database.connection(), EnqueueChange, ReadChanges, UpdateChange);

    AniListPendingChange missing;
    missing.id = 999;
    QString error;
    QVERIFY(!repository.updateStatus(missing, error));
    QVERIFY(!error.isEmpty());
}

QTEST_MAIN(SqliteRepositoryTests)
#include "SqliteRepositoryTests.moc"
