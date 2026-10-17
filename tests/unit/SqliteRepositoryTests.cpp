#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/database/SqliteDatabase.h"
#include "../../src/infrastructure/database/SqliteMediaRepository.h"
#include "../../src/infrastructure/database/SqlitePendingChangeRepository.h"
#include "../../src/infrastructure/database/SqlitePersonalListChangeWriter.h"

namespace {
const auto UpsertMedia = QStringLiteral(":/sqlite/queries/upsert-media.sql");
const auto ReadMedia = QStringLiteral(":/sqlite/queries/read-media.sql");
const auto ReadActiveMediaIds = QStringLiteral(":/sqlite/queries/read-active-media-ids.sql");
const auto MarkSourceRemoved = QStringLiteral(":/sqlite/queries/mark-media-source-removed.sql");
const auto UpdatePersonalListMedia = QStringLiteral(":/sqlite/queries/update-personal-list-media.sql");
const auto MarkPersonalListRemoved = QStringLiteral(":/sqlite/queries/mark-media-personal-list-removed.sql");
const auto SupersedePendingDeletion = QStringLiteral(":/sqlite/queries/supersede-pending-deletion.sql");
const auto EnqueueChange = QStringLiteral(":/sqlite/queries/enqueue-pending-change.sql");
const auto ReadChanges = QStringLiteral(":/sqlite/queries/read-pending-changes.sql");
const auto ReadPendingMediaIds = QStringLiteral(":/sqlite/queries/read-pending-media-ids.sql");
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
    void personalListChangePersistsMediaAndPendingChangesTogether();
    void pendingChangeRoundTripPreservesRemoteObservedAt();
    void readsDistinctMediaIdsWithAutomaticallySendableChanges();
    void updatingStatusWithNullLastErrorStoresEmptyText();
    void malformedPendingValueFailsInsteadOfBecomingEmptyText();
    void fractionalPendingIntegerIsRejected();
    void updatingMissingPendingChangeFails();
    void authoritativeSnapshotMarksOnlyUnseenActiveMedia();
    void removedMediaIsHiddenAndUpsertReactivatesItPreservingLocalData();
    void authoritativeSnapshotSkipsExcludedMediaAndHidesOnlyMarkedOnes();
    void readsMediaIdsWithUnsentChangesRegardlessOfOwner();
    void failedPendingChangeExcludesAbsentMediaFromRemoval();
    void pendingReadsAreScopedToCurrentOwnerAndKeepOtherOwnersQueued();
    void adoptingOrphanChangesAssignsOwnerAndReportsCount();
    void personalListWriterStoresOwnerOfPendingChanges();
    void removalHidesMediaAndQueuesDeletionInOneTransaction();
    void removalRollsBackMarkWhenDeletionCannotBeQueued();
    void synchronizationUpsertDoesNotRestoreRemovedMedia();
    void personalListUpdateRestoresRemovedMediaAndSupersedesPendingDeletion();
    void synchronizationUpsertKeepsUserAlternativeNamesAndAppendsSourceNames();
    void synchronizationUpsertWithSameNamesLeavesAlternativeNamesUnchanged();
    void editorUpdateStillReplacesAndRemovesAlternativeNames();
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
    QVERIFY2(repository.reconcileAuthoritativeSnapshot(QSet<int>{7}, QSet<int>{}, removedCount, error),
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

void SqliteRepositoryTests::personalListChangePersistsMediaAndPendingChangesTogether() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    QVERIFY(insertMedia(database.connection(), 154587));
    SqlitePersonalListChangeWriter writer(database.connection(), UpsertMedia, UpdatePersonalListMedia,
                                          EnqueueChange, MarkPersonalListRemoved, SupersedePendingDeletion);
    Media media;
    media.Id = 154587;
    media.ConsumedChapters = 12;
    media.PersonalScore = 9;
    media.ListStatus = UserListStatus::Completed;
    AniListPendingChange change;
    change.mediaId = media.Id;
    change.field = AniListField::Progress;
    change.previousValue = 4;
    change.newValue = 12;
    change.createdAt = QDateTime::currentDateTimeUtc();
    change.localUpdatedAt = change.createdAt;
    QString error;

    QVERIFY2(writer.save(media, {change}, error), qPrintable(error));

    QSqlQuery localMedia(database.connection());
    QVERIFY(localMedia.exec(QStringLiteral("SELECT consumed_chapters, personal_score, user_list_status FROM media WHERE id = 154587")));
    QVERIFY(localMedia.next());
    QCOMPARE(localMedia.value(0).toInt(), 12);
    QCOMPARE(localMedia.value(1).toInt(), 9);
    QCOMPARE(localMedia.value(2).toInt(), static_cast<int>(UserListStatus::Completed));
    QSqlQuery pending(database.connection());
    QVERIFY(pending.exec(QStringLiteral("SELECT COUNT(*) FROM anilist_pending_changes WHERE media_id = 154587")));
    QVERIFY(pending.next());
    QCOMPARE(pending.value(0).toInt(), 1);
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

void SqliteRepositoryTests::readsDistinctMediaIdsWithAutomaticallySendableChanges() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    for (const int mediaId : {7, 8, 9}) QVERIFY(insertMedia(database.connection(), mediaId));
    QSqlQuery insert(database.connection());
    QVERIFY(insert.exec(QStringLiteral(
        "INSERT INTO anilist_pending_changes "
        "(media_id, field, previous_value, new_value, created_at, local_updated_at, status) VALUES "
        "(7, 5, '{\"type\":\"int\",\"value\":1}', '{\"type\":\"int\",\"value\":2}', "
        "'2026-10-10T10:00:00Z', '2026-10-10T10:00:00Z', 0), "
        "(7, 5, '{\"type\":\"int\",\"value\":2}', '{\"type\":\"int\",\"value\":3}', "
        "'2026-10-10T10:01:00Z', '2026-10-10T10:01:00Z', 3), "
        "(8, 9, '{\"type\":\"text\",\"value\":\"\"}', '{\"type\":\"text\",\"value\":\"delete\"}', "
        "'2026-10-10T10:02:00Z', '2026-10-10T10:02:00Z', 4), "
        "(9, 5, '{\"type\":\"int\",\"value\":1}', '{\"type\":\"int\",\"value\":2}', "
        "'2026-10-10T10:03:00Z', '2026-10-10T10:03:00Z', 2)")));
    SqlitePendingChangeRepository repository(database.connection(), EnqueueChange, ReadChanges,
                                              ReadPendingMediaIds, UpdateChange);
    QList<int> mediaIds;
    QString error;

    QVERIFY2(repository.getPendingMediaIds(mediaIds, error), qPrintable(error));
    QCOMPARE(mediaIds, QList<int>{7});
}

void SqliteRepositoryTests::updatingStatusWithNullLastErrorStoresEmptyText() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    QVERIFY(insertMedia(database.connection(), 7));
    SqlitePendingChangeRepository repository(
        database.connection(), EnqueueChange, ReadChanges, UpdateChange);

    AniListPendingChange change;
    change.mediaId = 7;
    change.field = AniListField::Progress;
    change.previousValue = 1;
    change.newValue = 2;
    change.createdAt = QDateTime::fromString(QStringLiteral("2026-10-10T10:00:00Z"), Qt::ISODate);
    change.localUpdatedAt = change.createdAt;
    QString error;
    QVERIFY2(repository.enqueue(change, error), qPrintable(error));

    QList<AniListPendingChange> pending;
    QVERIFY2(repository.getPending(7, pending, error), qPrintable(error));
    QCOMPARE(pending.size(), 1);
    change = pending.first();
    change.status = AniListPendingChangeStatus::Succeeded;
    change.lastError.clear();

    QVERIFY2(repository.updateStatus(change, error), qPrintable(error));
    QSqlQuery persisted(database.connection());
    QVERIFY(persisted.exec(QStringLiteral(
        "SELECT status, last_error FROM anilist_pending_changes WHERE id = %1").arg(change.id)));
    QVERIFY(persisted.next());
    QCOMPARE(persisted.value(0).toInt(), static_cast<int>(AniListPendingChangeStatus::Succeeded));
    QCOMPARE(persisted.value(1).toString(), QStringLiteral(""));
    QVERIFY(!persisted.value(1).isNull());
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

void SqliteRepositoryTests::authoritativeSnapshotSkipsExcludedMediaAndHidesOnlyMarkedOnes() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    for (const int mediaId : {7, 8, 9}) QVERIFY(insertMedia(database.connection(), mediaId));
    SqliteMediaRepository repository(database.connection(), UpsertMedia, ReadMedia,
                                     ReadActiveMediaIds, MarkSourceRemoved);
    int removedCount = -1;
    QString error;

    QVERIFY2(repository.reconcileAuthoritativeSnapshot(QSet<int>{7}, QSet<int>{8}, removedCount, error),
             qPrintable(error));

    QCOMPARE(removedCount, 1);
    QList<Media> visible;
    QVERIFY(repository.readAll(visible, error));
    QList<int> visibleIds;
    for (const auto &media : visible) visibleIds.append(media.Id);
    std::sort(visibleIds.begin(), visibleIds.end());
    QCOMPARE(visibleIds, (QList<int>{7, 8}));
    Media returning;
    returning.Id = 9;
    returning.Name = QStringLiteral("Returned");
    QVERIFY(repository.upsert({returning}, error));
    QVERIFY(repository.readAll(visible, error));
    QCOMPARE(visible.size(), 3);
}

void SqliteRepositoryTests::readsMediaIdsWithUnsentChangesRegardlessOfOwner() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    for (const int mediaId : {7, 8, 9, 10, 11}) QVERIFY(insertMedia(database.connection(), mediaId));
    QSqlQuery insert(database.connection());
    QVERIFY(insert.exec(QStringLiteral(
        "INSERT INTO anilist_pending_changes "
        "(media_id, field, previous_value, new_value, created_at, local_updated_at, status) VALUES "
        "(7, 5, '{}', '{}', '2026-10-10T10:00:00Z', '2026-10-10T10:00:00Z', 0), "
        "(7, 5, '{}', '{}', '2026-10-10T10:01:00Z', '2026-10-10T10:01:00Z', 3), "
        "(8, 9, '{}', '{}', '2026-10-10T10:02:00Z', '2026-10-10T10:02:00Z', 4), "
        "(9, 5, '{}', '{}', '2026-10-10T10:03:00Z', '2026-10-10T10:03:00Z', 2), "
        "(10, 5, '{}', '{}', '2026-10-10T10:04:00Z', '2026-10-10T10:04:00Z', 1)")));
    SqlitePendingChangeRepository repository(database.connection(), EnqueueChange, ReadChanges,
                                              ReadPendingMediaIds, UpdateChange);
    QSet<int> mediaIds;
    QString error;

    QVERIFY2(repository.getMediaIdsWithUnsentChanges(mediaIds, error), qPrintable(error));

    QCOMPARE(mediaIds, (QSet<int>{7, 8, 10}));
}

void SqliteRepositoryTests::failedPendingChangeExcludesAbsentMediaFromRemoval() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    for (const int mediaId : {7, 8}) QVERIFY(insertMedia(database.connection(), mediaId));
    QSqlQuery insert(database.connection());
    QVERIFY(insert.exec(QStringLiteral(
        "INSERT INTO anilist_pending_changes "
        "(media_id, field, previous_value, new_value, created_at, local_updated_at, status) VALUES "
        "(8, 5, '{}', '{}', '2026-10-10T10:00:00Z', '2026-10-10T10:00:00Z', 3)")));
    SqlitePendingChangeRepository pendingChanges(database.connection(), EnqueueChange, ReadChanges,
                                                 ReadPendingMediaIds, UpdateChange);
    SqliteMediaRepository repository(database.connection(), UpsertMedia, ReadMedia,
                                     ReadActiveMediaIds, MarkSourceRemoved);
    QSet<int> excluded;
    int removedCount = -1;
    QString error;
    QVERIFY2(pendingChanges.getMediaIdsWithUnsentChanges(excluded, error), qPrintable(error));

    QVERIFY2(repository.reconcileAuthoritativeSnapshot(QSet<int>{}, excluded, removedCount, error),
             qPrintable(error));

    QCOMPARE(removedCount, 1);
    QSqlQuery rows(database.connection());
    QVERIFY(rows.exec(QStringLiteral("SELECT id FROM media WHERE source_removed_at IS NULL")));
    QVERIFY(rows.next());
    QCOMPARE(rows.value(0).toInt(), 8);
}

namespace {
AniListPendingChange ownedChange(int mediaId, qint64 owner) {
    AniListPendingChange change;
    change.mediaId = mediaId;
    change.field = AniListField::Progress;
    change.previousValue = 1;
    change.newValue = 2;
    change.createdAt = QDateTime::fromString(QStringLiteral("2026-10-10T10:00:00Z"), Qt::ISODate);
    change.localUpdatedAt = change.createdAt;
    change.ownerUserId = owner;
    return change;
}
}

void SqliteRepositoryTests::pendingReadsAreScopedToCurrentOwnerAndKeepOtherOwnersQueued() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    for (const int mediaId : {7, 8, 9}) QVERIFY(insertMedia(database.connection(), mediaId));
    SqlitePendingChangeRepository repository(database.connection(), EnqueueChange, ReadChanges,
                                              ReadPendingMediaIds, UpdateChange);
    QString error;
    QVERIFY2(repository.enqueue(ownedChange(7, 1), error), qPrintable(error));
    QVERIFY2(repository.enqueue(ownedChange(8, 2), error), qPrintable(error));
    QVERIFY2(repository.enqueue(ownedChange(9, 0), error), qPrintable(error));
    QList<int> mediaIds;
    QList<AniListPendingChange> changes;
    QSet<int> unsent;

    repository.setOwnerUserId(2);
    QVERIFY2(repository.getPendingMediaIds(mediaIds, error), qPrintable(error));
    QCOMPARE(mediaIds, QList<int>{8});
    QVERIFY2(repository.getPending(7, changes, error), qPrintable(error));
    QVERIFY(changes.isEmpty());
    QVERIFY2(repository.getPending(8, changes, error), qPrintable(error));
    QCOMPARE(changes.size(), 1);
    QCOMPARE(changes.first().ownerUserId, qint64(2));
    QVERIFY2(repository.getMediaIdsWithUnsentChanges(unsent, error), qPrintable(error));
    QCOMPARE(unsent, (QSet<int>{7, 8, 9}));

    repository.setOwnerUserId(1);
    QVERIFY2(repository.getPendingMediaIds(mediaIds, error), qPrintable(error));
    QCOMPARE(mediaIds, QList<int>{7});

    repository.setOwnerUserId(0);
    QVERIFY2(repository.getPendingMediaIds(mediaIds, error), qPrintable(error));
    QCOMPARE(mediaIds, QList<int>{9});
}

void SqliteRepositoryTests::adoptingOrphanChangesAssignsOwnerAndReportsCount() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    for (const int mediaId : {7, 8, 9}) QVERIFY(insertMedia(database.connection(), mediaId));
    SqlitePendingChangeRepository repository(database.connection(), EnqueueChange, ReadChanges,
                                              ReadPendingMediaIds, UpdateChange);
    QString error;
    QVERIFY2(repository.enqueue(ownedChange(7, 0), error), qPrintable(error));
    QVERIFY2(repository.enqueue(ownedChange(8, 0), error), qPrintable(error));
    QVERIFY2(repository.enqueue(ownedChange(9, 1), error), qPrintable(error));
    int adopted = -1;

    QVERIFY2(repository.adoptOrphanChanges(2, adopted, error), qPrintable(error));

    QCOMPARE(adopted, 2);
    repository.setOwnerUserId(2);
    QList<int> mediaIds;
    QVERIFY2(repository.getPendingMediaIds(mediaIds, error), qPrintable(error));
    QCOMPARE(mediaIds, (QList<int>{7, 8}));
    QVERIFY2(repository.adoptOrphanChanges(2, adopted, error), qPrintable(error));
    QCOMPARE(adopted, 0);
    repository.setOwnerUserId(1);
    QVERIFY2(repository.getPendingMediaIds(mediaIds, error), qPrintable(error));
    QCOMPARE(mediaIds, QList<int>{9});
}

void SqliteRepositoryTests::personalListWriterStoresOwnerOfPendingChanges() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    QVERIFY(insertMedia(database.connection(), 7));
    SqlitePersonalListChangeWriter writer(database.connection(), UpsertMedia, UpdatePersonalListMedia, EnqueueChange,
                                          MarkPersonalListRemoved, SupersedePendingDeletion);
    Media media;
    media.Id = 7;
    media.Name = QStringLiteral("Media");
    QString error;

    QVERIFY2(writer.save(media, {ownedChange(7, 5)}, error), qPrintable(error));

    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral("SELECT owner_user_id FROM anilist_pending_changes WHERE media_id = 7")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toLongLong(), qint64(5));
}

namespace {
AniListPendingChange deletionOf(int mediaId) {
    AniListPendingChange change;
    change.mediaId = mediaId;
    change.field = AniListField::Deletion;
    change.previousValue = QStringLiteral("CURRENT");
    change.newValue = QStringLiteral("");
    change.createdAt = QDateTime::fromString(QStringLiteral("2026-10-16T10:00:00Z"), Qt::ISODate);
    change.localUpdatedAt = change.createdAt;
    change.status = AniListPendingChangeStatus::Pending;
    change.ownerUserId = 3;
    return change;
}

int visibleMediaCount(QSqlDatabase database) {
    SqliteMediaRepository repository(database, UpsertMedia, ReadMedia, ReadActiveMediaIds, MarkSourceRemoved);
    QList<Media> media;
    QString error;
    return repository.readAll(media, error) ? media.size() : -1;
}
}

void SqliteRepositoryTests::removalHidesMediaAndQueuesDeletionInOneTransaction() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    QVERIFY(insertMedia(database.connection(), 7));
    SqlitePersonalListChangeWriter writer(database.connection(), UpsertMedia, UpdatePersonalListMedia,
                                          EnqueueChange, MarkPersonalListRemoved, SupersedePendingDeletion);
    Media media;
    media.Id = 7;
    QString error;

    QVERIFY2(writer.remove(media, deletionOf(7), error), qPrintable(error));

    QCOMPARE(visibleMediaCount(database.connection()), 0);
    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral("SELECT personal_list_removed_at FROM media WHERE id = 7")));
    QVERIFY(query.next());
    QVERIFY(!query.value(0).isNull());
    QVERIFY(query.exec(QStringLiteral(
        "SELECT field, status, owner_user_id FROM anilist_pending_changes WHERE media_id = 7")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), static_cast<int>(AniListField::Deletion));
    QCOMPARE(query.value(1).toInt(), static_cast<int>(AniListPendingChangeStatus::Pending));
    QCOMPARE(query.value(2).toLongLong(), qint64(3));
    QVERIFY(!query.next());
}

void SqliteRepositoryTests::removalRollsBackMarkWhenDeletionCannotBeQueued() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    QVERIFY(insertMedia(database.connection(), 7));
    SqlitePersonalListChangeWriter writer(database.connection(), UpsertMedia, UpdatePersonalListMedia,
                                          EnqueueChange, MarkPersonalListRemoved, SupersedePendingDeletion);
    Media media;
    media.Id = 7;
    QString error;

    QVERIFY(!writer.remove(media, deletionOf(999), error));

    QVERIFY(!error.isEmpty());
    QCOMPARE(visibleMediaCount(database.connection()), 1);
    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral("SELECT personal_list_removed_at FROM media WHERE id = 7")));
    QVERIFY(query.next());
    QVERIFY(query.value(0).isNull());
}

void SqliteRepositoryTests::synchronizationUpsertDoesNotRestoreRemovedMedia() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    QVERIFY(insertMedia(database.connection(), 7));
    SqlitePersonalListChangeWriter writer(database.connection(), UpsertMedia, UpdatePersonalListMedia,
                                          EnqueueChange, MarkPersonalListRemoved, SupersedePendingDeletion);
    Media media;
    media.Id = 7;
    QString error;
    QVERIFY2(writer.remove(media, deletionOf(7), error), qPrintable(error));
    SqliteMediaRepository repository(database.connection(), UpsertMedia, ReadMedia,
                                     ReadActiveMediaIds, MarkSourceRemoved);
    media.Name = QStringLiteral("Synchronized again");

    QVERIFY2(repository.upsert({media}, error), qPrintable(error));

    QCOMPARE(visibleMediaCount(database.connection()), 0);
}

void SqliteRepositoryTests::personalListUpdateRestoresRemovedMediaAndSupersedesPendingDeletion() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    QVERIFY(insertMedia(database.connection(), 7));
    SqlitePersonalListChangeWriter writer(database.connection(), UpsertMedia, UpdatePersonalListMedia,
                                          EnqueueChange, MarkPersonalListRemoved, SupersedePendingDeletion);
    Media media;
    media.Id = 7;
    QString error;
    QVERIFY2(writer.remove(media, deletionOf(7), error), qPrintable(error));
    AniListPendingChange readd = ownedChange(7, 3);
    readd.field = AniListField::ListStatus;
    readd.previousValue = QStringLiteral("");
    readd.newValue = QStringLiteral("PLANNING");
    media.ListStatus = UserListStatus::Planning;

    QVERIFY2(writer.save(media, {readd}, error), qPrintable(error));

    QCOMPARE(visibleMediaCount(database.connection()), 1);
    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral(
        "SELECT field, status FROM anilist_pending_changes WHERE media_id = 7 ORDER BY id")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), static_cast<int>(AniListField::Deletion));
    QCOMPARE(query.value(1).toInt(), static_cast<int>(AniListPendingChangeStatus::Superseded));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), static_cast<int>(AniListField::ListStatus));
    QCOMPARE(query.value(1).toInt(), static_cast<int>(AniListPendingChangeStatus::Pending));
}


void SqliteRepositoryTests::synchronizationUpsertKeepsUserAlternativeNamesAndAppendsSourceNames() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    SqliteMediaRepository repository(database.connection(), UpsertMedia, ReadMedia,
                                     ReadActiveMediaIds, MarkSourceRemoved);

    Media stored;
    stored.Id = 191656;
    stored.Name = QStringLiteral("Seitokai ni mo Ana wa Aru!");
    stored.AlternativeNames = {QStringLiteral("Even the Student Council Has Its Holes"),
                               QStringLiteral("Source Alias")};
    QString error;
    QVERIFY2(repository.upsert({stored}, error), qPrintable(error));

    Media synced = stored;
    synced.AlternativeNames = {QStringLiteral("  source alias "), QStringLiteral("New Source Alias")};
    QVERIFY2(repository.upsert({synced}, error), qPrintable(error));

    QList<Media> actual;
    QVERIFY2(repository.readAll(actual, error), qPrintable(error));
    QCOMPARE(actual.size(), 1);
    QCOMPARE(actual.first().AlternativeNames,
             (QStringList{QStringLiteral("Even the Student Council Has Its Holes"),
                          QStringLiteral("Source Alias"), QStringLiteral("New Source Alias")}));
}

void SqliteRepositoryTests::synchronizationUpsertWithSameNamesLeavesAlternativeNamesUnchanged() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    SqliteMediaRepository repository(database.connection(), UpsertMedia, ReadMedia,
                                     ReadActiveMediaIds, MarkSourceRemoved);

    Media media;
    media.Id = 5;
    media.Name = QStringLiteral("Same");
    media.AlternativeNames = {QStringLiteral("A"), QStringLiteral("B")};
    QString error;
    QVERIFY2(repository.upsert({media}, error), qPrintable(error));
    QVERIFY2(repository.upsert({media, media}, error), qPrintable(error));

    QList<Media> actual;
    QVERIFY2(repository.readAll(actual, error), qPrintable(error));
    QCOMPARE(actual.size(), 1);
    QCOMPARE(actual.first().AlternativeNames, media.AlternativeNames);
}

void SqliteRepositoryTests::editorUpdateStillReplacesAndRemovesAlternativeNames() {
    QTemporaryDir temporaryDirectory;
    SqliteDatabase database(temporaryDirectory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    SqliteMediaRepository repository(database.connection(), UpsertMedia, ReadMedia,
                                     ReadActiveMediaIds, MarkSourceRemoved, UpdatePersonalListMedia);

    Media media;
    media.Id = 6;
    media.Name = QStringLiteral("Editable");
    media.AlternativeNames = {QStringLiteral("Keep"), QStringLiteral("Remove")};
    QString error;
    QVERIFY2(repository.upsert({media}, error), qPrintable(error));

    media.AlternativeNames = {QStringLiteral("Keep")};
    QVERIFY2(repository.updatePersonalListMedia(media, error), qPrintable(error));

    QList<Media> actual;
    QVERIFY2(repository.readAll(actual, error), qPrintable(error));
    QCOMPARE(actual.first().AlternativeNames, (QStringList{QStringLiteral("Keep")}));
}

QTEST_MAIN(SqliteRepositoryTests)
#include "SqliteRepositoryTests.moc"
