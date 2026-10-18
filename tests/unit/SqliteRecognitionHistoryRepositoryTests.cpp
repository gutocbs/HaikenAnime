#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/database/SqliteDatabase.h"
#include "../../src/infrastructure/database/SqliteRecognitionHistoryRepository.h"

namespace {
const QDateTime Created(QDate(2026, 10, 17), QTime(21, 0), QTimeZone::UTC);

SqliteRecognitionHistoryRepository repositoryFor(SqliteDatabase &database) {
    return {database.connection(), QStringLiteral(":/sqlite/queries/read-recognition-history.sql"),
            QStringLiteral(":/sqlite/queries/insert-recognition-record.sql"),
            QStringLiteral(":/sqlite/queries/update-recognition-record.sql")};
}

RecognitionRecord recordFor(const QString &sessionKey, const int episode,
                            const std::optional<int> mediaId = 21, const QString &pluginId = QStringLiteral("plex")) {
    const PlaybackSession session{.id = PlaybackSessionId::create(pluginId, sessionKey).value(),
                                  .userId = QStringLiteral("7"), .deviceId = QStringLiteral("tv")};
    const auto candidate = EpisodeCandidate::create(QStringLiteral("One Piece"), mediaId, 1, episode, 0.9).value();
    return RecognitionRecord::create(session, candidate, Created);
}
}

class SqliteRecognitionHistoryRepositoryTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void insertAssignsIdAndRoundTripsFields();
    void roundTripsCorrectionAndSyncFailureAcrossReopen();
    void rejectsDuplicateDeduplicationKey();
    void findsByDeduplicationKey();
    void listsNewestFirstWithCursorPagination();
    void filtersByStatusAndPlugin();
    void findsRecordsAwaitingSyncByEffectiveMediaAndEpisode();
    void keepsUnknownSeasonAndMediaAsNull();
    void migrationRecordsVersionTwentyFive();
    void removeDeletesRecordAndDiscardsOnlyMatchingQueuedProgress();
    void removeWithoutQueuedProgressLeavesTheQueue();

private:
    std::unique_ptr<QTemporaryDir> directory;
    std::unique_ptr<SqliteDatabase> database;
};

void SqliteRecognitionHistoryRepositoryTests::init() {
    directory = std::make_unique<QTemporaryDir>();
    database = std::make_unique<SqliteDatabase>(directory->filePath(QStringLiteral("history.sqlite")));
    QVERIFY(database->open());
    QVERIFY2(database->migrate(), qPrintable(database->lastError()));
}

void SqliteRecognitionHistoryRepositoryTests::cleanup() {
    database.reset();
    directory.reset();
}

void SqliteRecognitionHistoryRepositoryTests::insertAssignsIdAndRoundTripsFields() {
    auto repository = repositoryFor(*database);
    auto record = recordFor(QStringLiteral("abc"), 6);
    QVERIFY(record.transitionTo(RecognitionStatus::MarkedWatched, Created.addSecs(180)));
    record.markSyncPending(Created.addSecs(180));
    QString error;

    QVERIFY2(repository.insert(record, error), qPrintable(error));
    QVERIFY(record.id() > 0);
    QList<RecognitionRecord> found;
    QVERIFY2(repository.find({.recordId = record.id()}, found, error), qPrintable(error));

    QCOMPARE(found.size(), 1);
    const auto &stored = found.first();
    QCOMPARE(stored.sessionId().key(), QStringLiteral("plex:abc"));
    QCOMPARE(stored.userId(), QStringLiteral("7"));
    QCOMPARE(stored.deviceId(), QStringLiteral("tv"));
    QCOMPARE(stored.candidate().title(), QStringLiteral("One Piece"));
    QCOMPARE(stored.candidate().mediaId(), std::optional<int>(21));
    QCOMPARE(stored.candidate().season(), std::optional<int>(1));
    QCOMPARE(stored.candidate().episode(), 6);
    QCOMPARE(stored.candidate().confidence(), 0.9);
    QCOMPARE(stored.status(), RecognitionStatus::MarkedWatched);
    QCOMPARE(stored.syncState(), RecognitionSyncState::Pending);
    QCOMPARE(stored.createdAt(), Created);
    QCOMPARE(stored.updatedAt(), Created.addSecs(180));
    QCOMPARE(stored.deduplicationKey(), record.deduplicationKey());
}

void SqliteRecognitionHistoryRepositoryTests::roundTripsCorrectionAndSyncFailureAcrossReopen() {
    const auto path = database->databasePath();
    qint64 id = 0;
    QString error;
    {
        auto repository = repositoryFor(*database);
        auto record = recordFor(QStringLiteral("abc"), 6);
        QVERIFY(record.transitionTo(RecognitionStatus::Uncertain, Created));
        record.setDiagnostic(QStringLiteral("low-confidence"));
        QVERIFY(repository.insert(record, error));
        const auto correction = EpisodeCandidate::create(QStringLiteral("One Piece Film"), 99, std::nullopt, 1, 1.0).value();
        QVERIFY(record.correct(correction, Created.addSecs(60)));
        record.markSyncFailed(QStringLiteral("network"), Created.addSecs(90));
        QVERIFY2(repository.update(record, error), qPrintable(error));
        id = record.id();
    }
    database.reset();
    SqliteDatabase reopened(path);
    QVERIFY(reopened.open());
    QVERIFY2(reopened.migrate(), qPrintable(reopened.lastError()));
    auto repository = repositoryFor(reopened);
    QList<RecognitionRecord> found;

    QVERIFY2(repository.find({.recordId = id}, found, error), qPrintable(error));

    QCOMPARE(found.size(), 1);
    const auto &stored = found.first();
    QCOMPARE(stored.status(), RecognitionStatus::Corrected);
    QCOMPARE(stored.diagnostic(), QStringLiteral("low-confidence"));
    QVERIFY(stored.correction().has_value());
    QCOMPARE(stored.correction()->mediaId(), std::optional<int>(99));
    QCOMPARE(stored.correction()->season(), std::optional<int>());
    QCOMPARE(stored.syncState(), RecognitionSyncState::Failed);
    QCOMPARE(stored.syncError(), QStringLiteral("network"));
    QCOMPARE(stored.updatedAt(), Created.addSecs(90));
}

void SqliteRecognitionHistoryRepositoryTests::rejectsDuplicateDeduplicationKey() {
    auto repository = repositoryFor(*database);
    auto first = recordFor(QStringLiteral("abc"), 6);
    auto duplicate = recordFor(QStringLiteral("abc"), 6);
    QString error;
    QVERIFY(repository.insert(first, error));

    QVERIFY(!repository.insert(duplicate, error));

    QVERIFY(!error.isEmpty());
}

void SqliteRecognitionHistoryRepositoryTests::findsByDeduplicationKey() {
    auto repository = repositoryFor(*database);
    auto first = recordFor(QStringLiteral("abc"), 6);
    auto other = recordFor(QStringLiteral("abc"), 7);
    QString error;
    QVERIFY(repository.insert(first, error));
    QVERIFY(repository.insert(other, error));
    QList<RecognitionRecord> found;

    QVERIFY(repository.find({.deduplicationKey = first.deduplicationKey()}, found, error));

    QCOMPARE(found.size(), 1);
    QCOMPARE(found.first().id(), first.id());
}

void SqliteRecognitionHistoryRepositoryTests::listsNewestFirstWithCursorPagination() {
    auto repository = repositoryFor(*database);
    QString error;
    QList<qint64> ids;
    for (int episode = 1; episode <= 5; ++episode) {
        auto record = recordFor(QStringLiteral("abc"), episode);
        QVERIFY(repository.insert(record, error));
        ids.append(record.id());
    }
    QList<RecognitionRecord> firstPage;
    QList<RecognitionRecord> secondPage;

    QVERIFY(repository.find({.limit = 2}, firstPage, error));
    QVERIFY(repository.find({.beforeId = firstPage.last().id(), .limit = 2}, secondPage, error));

    QCOMPARE(firstPage.size(), 2);
    QCOMPARE(firstPage.at(0).id(), ids.at(4));
    QCOMPARE(firstPage.at(1).id(), ids.at(3));
    QCOMPARE(secondPage.size(), 2);
    QCOMPARE(secondPage.at(0).id(), ids.at(2));
    QCOMPARE(secondPage.at(1).id(), ids.at(1));
}

void SqliteRecognitionHistoryRepositoryTests::filtersByStatusAndPlugin() {
    auto repository = repositoryFor(*database);
    QString error;
    auto watched = recordFor(QStringLiteral("abc"), 1);
    QVERIFY(watched.transitionTo(RecognitionStatus::MarkedWatched, Created));
    auto pending = recordFor(QStringLiteral("abc"), 2);
    auto mpv = recordFor(QStringLiteral("pipe"), 3, 21, QStringLiteral("mpv"));
    QVERIFY(mpv.transitionTo(RecognitionStatus::MarkedWatched, Created));
    QVERIFY(repository.insert(watched, error));
    QVERIFY(repository.insert(pending, error));
    QVERIFY(repository.insert(mpv, error));
    QList<RecognitionRecord> found;

    QVERIFY(repository.find({.status = RecognitionStatus::MarkedWatched, .pluginId = QStringLiteral("plex")},
                            found, error));

    QCOMPARE(found.size(), 1);
    QCOMPARE(found.first().id(), watched.id());
}

void SqliteRecognitionHistoryRepositoryTests::findsRecordsAwaitingSyncByEffectiveMediaAndEpisode() {
    // Arrange
    auto repository = repositoryFor(*database);
    QString error;
    auto pending = recordFor(QStringLiteral("a"), 4);
    pending.markSyncPending(Created);
    auto failed = recordFor(QStringLiteral("b"), 5);
    failed.markSyncFailed(QStringLiteral("falha"), Created);
    auto laterEpisode = recordFor(QStringLiteral("c"), 6);
    laterEpisode.markSyncPending(Created);
    auto synced = recordFor(QStringLiteral("d"), 3);
    synced.markSynced(Created);
    auto notRequired = recordFor(QStringLiteral("e"), 2);
    auto otherMedia = recordFor(QStringLiteral("f"), 1, 99);
    otherMedia.markSyncPending(Created);
    auto correctedAway = recordFor(QStringLiteral("g"), 1);
    QVERIFY(correctedAway.transitionTo(RecognitionStatus::Uncertain, Created));
    QVERIFY(correctedAway.correct(EpisodeCandidate::create(QStringLiteral("Other"), 99, 1, 1, 1.0).value(), Created));
    correctedAway.markSyncPending(Created);
    auto correctedHere = recordFor(QStringLiteral("h"), 9, 99);
    QVERIFY(correctedHere.transitionTo(RecognitionStatus::Uncertain, Created));
    QVERIFY(correctedHere.correct(EpisodeCandidate::create(QStringLiteral("One Piece"), 21, 1, 2, 1.0).value(),
                                  Created));
    correctedHere.markSyncPending(Created);
    for (auto *record : {&pending, &failed, &laterEpisode, &synced, &notRequired, &otherMedia, &correctedAway,
                         &correctedHere}) {
        QVERIFY2(repository.insert(*record, error), qPrintable(error));
    }
    QList<RecognitionRecord> found;

    // Act
    QVERIFY2(repository.find({.mediaId = 21, .maxEpisode = 5, .awaitingSync = true, .limit = 100}, found, error),
             qPrintable(error));

    // Assert
    QList<qint64> ids;
    for (const auto &record : found) ids.append(record.id());
    QCOMPARE(ids, (QList<qint64>{correctedHere.id(), failed.id(), pending.id()}));
}

void SqliteRecognitionHistoryRepositoryTests::keepsUnknownSeasonAndMediaAsNull() {
    auto repository = repositoryFor(*database);
    const PlaybackSession session{.id = PlaybackSessionId::create(QStringLiteral("mpv"), QStringLiteral("p")).value()};
    auto record = RecognitionRecord::create(
        session, EpisodeCandidate::create(QStringLiteral("Unknown Show"), std::nullopt, std::nullopt, 2, 0.5).value(),
        Created);
    QString error;
    QVERIFY(repository.insert(record, error));
    QList<RecognitionRecord> found;

    QVERIFY(repository.find({.recordId = record.id()}, found, error));

    QCOMPARE(found.first().candidate().mediaId(), std::optional<int>());
    QCOMPARE(found.first().candidate().season(), std::optional<int>());
    QVERIFY(!found.first().correction().has_value());
}

void SqliteRecognitionHistoryRepositoryTests::migrationRecordsVersionTwentyFive() {
    QSqlQuery query(database->connection());

    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM schema_version WHERE version = 25")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
}

namespace {
void insertPendingChange(QSqlQuery &query, const int id, const int field, const int episode, const int status,
                         const qint64 owner) {
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO anilist_pending_changes (id, media_id, field, previous_value, new_value, created_at, "
        "local_updated_at, status, owner_user_id) VALUES (%1, 21, %2, '{\"type\":\"int\",\"value\":5}', "
        "'{\"type\":\"int\",\"value\":%3}', '2026-10-17T00:00:00Z', '2026-10-17T00:00:00Z', %4, %5)")
                           .arg(id).arg(field).arg(episode).arg(status).arg(owner)));
}

int statusOf(QSqlQuery &query, const int id) {
    if (!query.exec(QStringLiteral("SELECT status FROM anilist_pending_changes WHERE id = %1").arg(id)) || !query.next()) {
        return -1;
    }
    return query.value(0).toInt();
}
}

void SqliteRecognitionHistoryRepositoryTests::removeDeletesRecordAndDiscardsOnlyMatchingQueuedProgress() {
    auto repository = repositoryFor(*database);
    QSqlQuery query(database->connection());
    QVERIFY(query.exec(QStringLiteral("INSERT INTO media (id, name, type, status) VALUES (21, 'One Piece', 1, 1)")));
    constexpr int progress = 5;
    constexpr int score = 6;
    insertPendingChange(query, 1, progress, 6, 0, 99);
    insertPendingChange(query, 2, progress, 6, 3, 0);
    insertPendingChange(query, 3, progress, 6, 1, 99);
    insertPendingChange(query, 4, progress, 7, 0, 99);
    insertPendingChange(query, 5, score, 6, 0, 99);
    auto record = recordFor(QStringLiteral("abc"), 6);
    QString error;
    QVERIFY(repository.insert(record, error));

    QVERIFY2(repository.remove(record.id(), QueuedProgress{.mediaId = 21, .episode = 6}, error), qPrintable(error));

    QList<RecognitionRecord> found;
    QVERIFY(repository.find({.recordId = record.id()}, found, error));
    QVERIFY(found.isEmpty());
    constexpr int superseded = 5;
    QCOMPARE(statusOf(query, 1), superseded);
    QCOMPARE(statusOf(query, 2), superseded);
    QCOMPARE(statusOf(query, 3), 1);
    QCOMPARE(statusOf(query, 4), 0);
    QCOMPARE(statusOf(query, 5), 0);
}

void SqliteRecognitionHistoryRepositoryTests::removeWithoutQueuedProgressLeavesTheQueue() {
    auto repository = repositoryFor(*database);
    QSqlQuery query(database->connection());
    QVERIFY(query.exec(QStringLiteral("INSERT INTO media (id, name, type, status) VALUES (21, 'One Piece', 1, 1)")));
    insertPendingChange(query, 1, 5, 6, 0, 99);
    auto record = recordFor(QStringLiteral("abc"), 6);
    QString error;
    QVERIFY(repository.insert(record, error));

    QVERIFY2(repository.remove(record.id(), std::nullopt, error), qPrintable(error));

    QCOMPARE(statusOf(query, 1), 0);
    QVERIFY(!repository.remove(record.id(), std::nullopt, error));
}

QTEST_MAIN(SqliteRecognitionHistoryRepositoryTests)
#include "SqliteRecognitionHistoryRepositoryTests.moc"
