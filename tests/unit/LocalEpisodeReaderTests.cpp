#include <QSqlQuery>
#include <QSqlError>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/application/library/LocalEpisodeReader.h"
#include "../../src/infrastructure/database/SqliteDatabase.h"

class LocalEpisodeReaderTests final : public QObject {
    Q_OBJECT
private slots:
    void selectsSmallestEligibleEpisodeAndDeterministicCopy();
    void skipsUnavailableAndNonAssociatedFiles();
    void returnsNoEpisodeWhenProgressIsCurrent();
};

namespace {
void seedMedia(QSqlDatabase database, int id) {
    QSqlQuery query(database);
    QVERIFY(query.exec(QStringLiteral("INSERT INTO media (id, name, alternative_names, type, status) "
                                      "VALUES (%1, 'Example', '[]', 0, 0)").arg(id)));
}

void seedScan(QSqlDatabase database) {
    QSqlQuery query(database);
    QVERIFY(query.exec(QStringLiteral("INSERT INTO library_scans (id, root_path, started_at, status) "
                                      "VALUES (1, 'Q:\\', '2026-10-02T00:00:00Z', 'succeeded')")));
}

void seedFile(QSqlDatabase database, int id, int mediaId, int episode, QString path,
              QString state = QStringLiteral("associated"), int available = 1) {
    QSqlQuery query(database);
    query.prepare(QStringLiteral("INSERT INTO local_files "
        "(id, root_path, relative_path, normalized_relative_path, file_name, extension, size_bytes, modified_at, "
        "available, last_seen_scan_id, recognition_state, media_kind, episode, media_id) "
        "VALUES (:id, 'Q:\\', :path, :path, :path, '.mkv', 1, '2026-10-02T00:00:00Z', :available, 1, :state, 'anime', :episode, :media_id)"));
    query.bindValue(QStringLiteral(":id"), id);
    query.bindValue(QStringLiteral(":path"), path);
    query.bindValue(QStringLiteral(":available"), available);
    query.bindValue(QStringLiteral(":state"), state);
    query.bindValue(QStringLiteral(":episode"), episode);
    query.bindValue(QStringLiteral(":media_id"), mediaId);
    QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
}

LocalEpisodeReader reader(QSqlDatabase database) {
    return LocalEpisodeReader(database,
        QStringLiteral(":/sqlite/queries/read-next-local-episode.sql"),
        QStringLiteral(":/sqlite/queries/read-available-episode-count.sql"));
}
}

void LocalEpisodeReaderTests::selectsSmallestEligibleEpisodeAndDeterministicCopy() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open()); QVERIFY(database.migrate()); seedScan(database.connection()); seedMedia(database.connection(), 10);
    seedFile(database.connection(), 1, 10, 3, QStringLiteral("Z/03.mkv"));
    seedFile(database.connection(), 2, 10, 3, QStringLiteral("A/03.mkv"));
    seedFile(database.connection(), 3, 10, 4, QStringLiteral("04.mkv"));
    auto episodeReader = reader(database.connection()); LocalEpisode episode; QString error;
    QVERIFY2(episodeReader.readNextEpisode(10, 2, episode, error), qPrintable(error));
    QCOMPARE(episode.episode, 3); QCOMPARE(episode.path, QStringLiteral("Q:\\A/03.mkv"));
}

void LocalEpisodeReaderTests::skipsUnavailableAndNonAssociatedFiles() {
    QTemporaryDir directory; SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open()); QVERIFY(database.migrate()); seedScan(database.connection()); seedMedia(database.connection(), 10);
    seedFile(database.connection(), 1, 10, 1, QStringLiteral("01.mkv"), QStringLiteral("associated"), 0);
    seedFile(database.connection(), 2, 10, 2, QStringLiteral("02.mkv"), QStringLiteral("ambiguous"));
    seedFile(database.connection(), 3, 10, 3, QStringLiteral("03.mkv"));
    auto episodeReader = reader(database.connection()); LocalEpisode episode; QString error;
    QVERIFY2(episodeReader.readNextEpisode(10, 0, episode, error), qPrintable(error));
    QCOMPARE(episode.episode, 3);
}

void LocalEpisodeReaderTests::returnsNoEpisodeWhenProgressIsCurrent() {
    QTemporaryDir directory; SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open()); QVERIFY(database.migrate()); seedScan(database.connection()); seedMedia(database.connection(), 10);
    seedFile(database.connection(), 1, 10, 3, QStringLiteral("03.mkv"));
    auto episodeReader = reader(database.connection()); LocalEpisode episode; QString error;
    QVERIFY(!episodeReader.readNextEpisode(10, 3, episode, error)); QVERIFY(error.isEmpty());
}

QTEST_MAIN(LocalEpisodeReaderTests)
#include "LocalEpisodeReaderTests.moc"
