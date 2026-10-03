#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/database/SqliteDatabase.h"
#include "../../src/infrastructure/database/SqliteMediaRepository.h"
#include "../../src/infrastructure/database/SqlitePendingChangeRepository.h"

namespace {
const auto UpsertMedia = QStringLiteral(":/sqlite/queries/upsert-media.sql");
const auto ReadMedia = QStringLiteral(":/sqlite/queries/read-media.sql");
const auto ReadActiveMediaIds = QStringLiteral(":/sqlite/queries/read-active-media-ids.sql");
const auto MarkSourceRemoved = QStringLiteral(":/sqlite/queries/mark-media-source-removed.sql");
const auto UpdatePersonalListMedia = QStringLiteral(":/sqlite/queries/update-personal-list-media.sql");
const auto EnqueueChange = QStringLiteral(":/sqlite/queries/enqueue-pending-change.sql");
const auto ReadChanges = QStringLiteral(":/sqlite/queries/read-pending-changes.sql");
const auto UpdateChange = QStringLiteral(":/sqlite/queries/update-pending-change.sql");

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
    void mediaRoundTripPreservesExtendedMetadata();
    void extendedMetadataUpsertPreservesUserEditedFields();
    void personalListUpdatePersistsEditorFieldsWithoutOverwritingCatalogMetadata();
    void pendingChangeRoundTripPreservesRemoteObservedAt();
    void malformedPendingValueFailsInsteadOfBecomingEmptyText();
    void fractionalPendingIntegerIsRejected();
    void updatingMissingPendingChangeFails();
    void authoritativeSnapshotMarksOnlyUnseenActiveMedia();
    void removedMediaIsHiddenAndUpsertReactivatesItPreservingLocalData();
};

void SqliteRepositoryTests::mediaRoundTripPreservesAlternativeNames() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    SqliteMediaRepository repository(database.connection(), UpsertMedia, ReadMedia,
                                     ReadActiveMediaIds, MarkSourceRemoved);

    Media expected;
    expected.Id = 7;
    expected.Name = QStringLiteral("Primary");
    expected.AlternativeNames = {QStringLiteral("Alias A"), QStringLiteral("Alias B")};
    expected.ListStatus = UserListStatus::Completed;
    expected.CoverMediumUrl = QStringLiteral("https://img/medium.jpg");
    expected.CoverLargeUrl = QStringLiteral("https://img/large.jpg");
    expected.CoverExtraLargeUrl = QStringLiteral("https://img/extra.jpg");
    QString error;
    QVERIFY(repository.upsert({expected}, error));

    QList<Media> actual;
    QVERIFY(repository.readAll(actual, error));
    QCOMPARE(actual.size(), 1);
    QCOMPARE(actual.first().AlternativeNames, expected.AlternativeNames);
    QCOMPARE(actual.first().ListStatus, expected.ListStatus);
    QCOMPARE(actual.first().CoverMediumUrl, expected.CoverMediumUrl);
    QCOMPARE(actual.first().CoverLargeUrl, expected.CoverLargeUrl);
    QCOMPARE(actual.first().CoverExtraLargeUrl, expected.CoverExtraLargeUrl);
}

void SqliteRepositoryTests::mediaRoundTripPreservesExtendedMetadata() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    SqliteMediaRepository repository(database.connection(), UpsertMedia, ReadMedia,
                                     ReadActiveMediaIds, MarkSourceRemoved);

    Media expected;
    expected.Id = 77;
    expected.Name = QStringLiteral("Extended");
    expected.Season = QStringLiteral("WINTER");
    expected.SeasonYear = 2026;
    expected.NextAiringEpisode = 11;
    expected.NextAiringAt = qint64(1790518560);
    expected.AniListUrl = QStringLiteral("https://anilist.co/anime/77");
    expected.ExternalLinks = {
        MediaLink{QStringLiteral("Official"), QStringLiteral("https://example.test/official")},
        MediaLink{QStringLiteral("Stream"), QStringLiteral("https://stream.example/watch")}
    };
    Media missing;
    missing.Id = 78;
    missing.Name = QStringLiteral("Missing optionals");
    QString error;
    QVERIFY2(repository.upsert({expected, missing}, error), qPrintable(error));

    QList<Media> actual;
    QVERIFY2(repository.readAll(actual, error), qPrintable(error));
    QCOMPARE(actual.size(), 2);
    QCOMPARE(actual.first().Season, expected.Season);
    QCOMPARE(actual.first().SeasonYear, expected.SeasonYear);
    QCOMPARE(actual.first().NextAiringEpisode, expected.NextAiringEpisode);
    QCOMPARE(actual.first().NextAiringAt, expected.NextAiringAt);
    QCOMPARE(actual.first().AniListUrl, expected.AniListUrl);
    QCOMPARE(actual.first().ExternalLinks.size(), 2);
    QCOMPARE(actual.first().ExternalLinks.at(0).Site, expected.ExternalLinks.at(0).Site);
    QCOMPARE(actual.first().ExternalLinks.at(0).Url, expected.ExternalLinks.at(0).Url);
    QCOMPARE(actual.first().ExternalLinks.at(1).Site, expected.ExternalLinks.at(1).Site);
    QCOMPARE(actual.first().ExternalLinks.at(1).Url, expected.ExternalLinks.at(1).Url);
    QVERIFY(actual.at(1).Season.isEmpty());
    QVERIFY(!actual.at(1).SeasonYear.has_value());
    QVERIFY(!actual.at(1).NextAiringEpisode.has_value());
    QVERIFY(!actual.at(1).NextAiringAt.has_value());
    QVERIFY(actual.at(1).AniListUrl.isEmpty());
    QVERIFY(actual.at(1).ExternalLinks.isEmpty());
}

void SqliteRepositoryTests::extendedMetadataUpsertPreservesUserEditedFields() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    SqliteMediaRepository repository(database.connection(), UpsertMedia, ReadMedia,
                                     ReadActiveMediaIds, MarkSourceRemoved);

    Media initial;
    initial.Id = 79;
    initial.Name = QStringLiteral("Before refresh");
    QString error;
    QVERIFY2(repository.upsert({initial}, error), qPrintable(error));

    QSqlQuery localEdit(database.connection());
    QVERIFY(localEdit.exec(QStringLiteral(
        "UPDATE media SET consumed_chapters = 6, next_chapter = 7, personal_score = 91, "
        "user_list_status = 4, local_path = 'D:\\Local\\media.mkv' "
        "WHERE id = 79")));

    Media refreshed;
    refreshed.Id = 79;
    refreshed.Name = QStringLiteral("After refresh");
    refreshed.ConsumedChapters = 28;
    refreshed.PersonalScore = 10;
    refreshed.Season = QStringLiteral("SPRING");
    refreshed.SeasonYear = 2026;
    refreshed.NextAiringEpisode = 8;
    refreshed.NextAiringAt = qint64(1790518560);
    refreshed.AniListUrl = QStringLiteral("https://anilist.co/anime/79");
    refreshed.ExternalLinks = {
        MediaLink{QStringLiteral("Official"), QStringLiteral("https://example.test/79")}
    };
    QVERIFY2(repository.upsert({refreshed}, error), qPrintable(error));

    QList<Media> actual;
    QVERIFY2(repository.readAll(actual, error), qPrintable(error));
    QCOMPARE(actual.size(), 1);
    QCOMPARE(actual.first().Name, QStringLiteral("After refresh"));
    QCOMPARE(actual.first().ConsumedChapters, 28);
    QCOMPARE(actual.first().NextChapter, 7);
    QCOMPARE(actual.first().PersonalScore, 10);
    QCOMPARE(actual.first().ListStatus, UserListStatus::Completed);
    QCOMPARE(actual.first().LocalPath, QStringLiteral("D:\\Local\\media.mkv"));
    QCOMPARE(actual.first().Season, QStringLiteral("SPRING"));
    QCOMPARE(actual.first().SeasonYear, std::optional<int>(2026));
    QCOMPARE(actual.first().NextAiringEpisode, std::optional<int>(8));
    QCOMPARE(actual.first().NextAiringAt, std::optional<qint64>(1790518560));
    QCOMPARE(actual.first().AniListUrl, QStringLiteral("https://anilist.co/anime/79"));
    QCOMPARE(actual.first().ExternalLinks.size(), 1);
    QCOMPARE(actual.first().ExternalLinks.first().Url, QStringLiteral("https://example.test/79"));
}

void SqliteRepositoryTests::personalListUpdatePersistsEditorFieldsWithoutOverwritingCatalogMetadata() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    SqliteMediaRepository repository(database.connection(), UpsertMedia, ReadMedia,
                                     ReadActiveMediaIds, MarkSourceRemoved, UpdatePersonalListMedia);

    Media existing;
    existing.Id = 80;
    existing.Name = QStringLiteral("Catalog title");
    existing.Synopsis = QStringLiteral("Catalog synopsis");
    existing.NextChapter = 11;
    existing.Season = QStringLiteral("SUMMER");
    existing.AlternativeNames = {QStringLiteral("Original title")};
    existing.LocalPath = QStringLiteral("D:\\Original\\catalog.mkv");
    QString error;
    QVERIFY2(repository.upsert({existing}, error), qPrintable(error));
    QSqlQuery localEdit(database.connection());
    QVERIFY(localEdit.exec(QStringLiteral("UPDATE media SET next_chapter = 11 WHERE id = 80")));

    Media edited = existing;
    edited.ConsumedChapters = 8;
    edited.PersonalScore = 7;
    edited.ListStatus = UserListStatus::Completed;
    edited.LocalPath = QStringLiteral("D:\\Anime\\edited.mkv");
    edited.AlternativeNames = {QStringLiteral("Edited title"), QStringLiteral("Localized title")};
    QVERIFY2(repository.updatePersonalListMedia(edited, error), qPrintable(error));

    QList<Media> actual;
    QVERIFY2(repository.readAll(actual, error), qPrintable(error));
    QCOMPARE(actual.size(), 1);
    QCOMPARE(actual.first().ConsumedChapters, edited.ConsumedChapters);
    QCOMPARE(actual.first().PersonalScore, edited.PersonalScore);
    QCOMPARE(actual.first().ListStatus, edited.ListStatus);
    QCOMPARE(actual.first().LocalPath, edited.LocalPath);
    QCOMPARE(actual.first().AlternativeNames, edited.AlternativeNames);
    QCOMPARE(actual.first().Name, existing.Name);
    QCOMPARE(actual.first().Synopsis, existing.Synopsis);
    QCOMPARE(actual.first().NextChapter, existing.NextChapter);
    QCOMPARE(actual.first().Season, existing.Season);
}

void SqliteRepositoryTests::authoritativeSnapshotMarksOnlyUnseenActiveMedia() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    QVERIFY(insertMedia(database.connection(), 7));
    QVERIFY(insertMedia(database.connection(), 8));
    QVERIFY(insertMedia(database.connection(), 9));
    QSqlQuery alreadyRemoved(database.connection());
    QVERIFY(alreadyRemoved.exec(QStringLiteral(
        "UPDATE media SET source_removed_at = '2026-09-24T00:00:00Z' WHERE id = 9")));
    SqliteMediaRepository repository(database.connection(), UpsertMedia, ReadMedia,
                                     ReadActiveMediaIds, MarkSourceRemoved);

    int removedCount = -1;
    QString error;
    QVERIFY2(repository.reconcileAuthoritativeSnapshot(QSet<int>{7}, removedCount, error),
             qPrintable(error));
    QCOMPARE(removedCount, 1);

    QSqlQuery rows(database.connection());
    QVERIFY(rows.exec(QStringLiteral("SELECT id, source_removed_at FROM media ORDER BY id")));
    QVERIFY(rows.next());
    QCOMPARE(rows.value(0).toInt(), 7);
    QVERIFY(rows.value(1).isNull());
    QVERIFY(rows.next());
    QCOMPARE(rows.value(0).toInt(), 8);
    QVERIFY(!rows.value(1).isNull());
    QVERIFY(rows.next());
    QCOMPARE(rows.value(0).toInt(), 9);
    QCOMPARE(rows.value(1).toString(), QStringLiteral("2026-09-24T00:00:00Z"));
}

void SqliteRepositoryTests::removedMediaIsHiddenAndUpsertReactivatesItPreservingLocalData() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    QVERIFY(insertMedia(database.connection(), 7));
    QSqlQuery local(database.connection());
    QVERIFY(local.exec(QStringLiteral(
        "UPDATE media SET consumed_chapters = 12, personal_score = 90, "
        "source_removed_at = '2026-09-24T00:00:00Z' WHERE id = 7")));
    SqliteMediaRepository repository(database.connection(), UpsertMedia, ReadMedia,
                                     ReadActiveMediaIds, MarkSourceRemoved);
    QString error;
    QList<Media> active;
    QVERIFY(repository.readAll(active, error));
    QVERIFY(active.isEmpty());

    Media returning;
    returning.Id = 7;
    returning.Name = QStringLiteral("Returned");
    returning.ConsumedChapters = 12;
    returning.PersonalScore = 90;
    returning.CoverUrl = QStringLiteral("https://example.test/new.jpg");
    QVERIFY(repository.upsert({returning}, error));
    QVERIFY(repository.readAll(active, error));
    QCOMPARE(active.size(), 1);
    QCOMPARE(active.first().ConsumedChapters, 12);
    QCOMPARE(active.first().PersonalScore, 90);
    QCOMPARE(active.first().CoverUrl, returning.CoverUrl);
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
