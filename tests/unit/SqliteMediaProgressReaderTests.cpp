#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/database/SqliteDatabase.h"
#include "../../src/infrastructure/database/SqliteMediaProgressReader.h"

class SqliteMediaProgressReaderTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void readsTheCurrentProgressOfOneMedia();
    void unknownMediaIsNotAnError();
    void removedMediaIsNotFound();

private:
    std::unique_ptr<QTemporaryDir> directory;
    std::unique_ptr<SqliteDatabase> database;
};

void SqliteMediaProgressReaderTests::init() {
    directory = std::make_unique<QTemporaryDir>();
    database = std::make_unique<SqliteDatabase>(directory->filePath(QStringLiteral("app.sqlite")));
    QVERIFY2(database->open(), qPrintable(database->lastError()));
    QVERIFY2(database->migrate(), qPrintable(database->lastError()));
    QSqlQuery query(database->connection());
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO media (id, name, type, status, consumed_chapters, total_chapters) "
        "VALUES (21, 'One Piece', 1, 1, 5, 12), (22, 'Gone', 1, 1, 1, 12)")));
    QVERIFY(query.exec(QStringLiteral("UPDATE media SET source_removed_at = '2026-10-17T00:00:00Z' WHERE id = 22")));
}

void SqliteMediaProgressReaderTests::cleanup() {
    database.reset();
    directory.reset();
}

void SqliteMediaProgressReaderTests::readsTheCurrentProgressOfOneMedia() {
    SqliteMediaProgressReader reader(database->connection());
    QSqlQuery query(database->connection());
    QVERIFY(query.exec(QStringLiteral("UPDATE media SET consumed_chapters = 6 WHERE id = 21")));
    std::optional<Media> media;
    QString error;

    QVERIFY2(reader.find(21, media, error), qPrintable(error));

    QVERIFY(media.has_value());
    QCOMPARE(media->Id, 21);
    QCOMPARE(media->Name, QStringLiteral("One Piece"));
    QCOMPARE(media->ConsumedChapters, 6);
    QCOMPARE(media->TotalChapters, 12);
}

void SqliteMediaProgressReaderTests::unknownMediaIsNotAnError() {
    SqliteMediaProgressReader reader(database->connection());
    std::optional<Media> media = Media();
    QString error;

    QVERIFY2(reader.find(999, media, error), qPrintable(error));

    QVERIFY(!media.has_value());
}

void SqliteMediaProgressReaderTests::removedMediaIsNotFound() {
    SqliteMediaProgressReader reader(database->connection());
    std::optional<Media> media;
    QString error;

    QVERIFY2(reader.find(22, media, error), qPrintable(error));

    QVERIFY(!media.has_value());
}

QTEST_MAIN(SqliteMediaProgressReaderTests)
#include "SqliteMediaProgressReaderTests.moc"
