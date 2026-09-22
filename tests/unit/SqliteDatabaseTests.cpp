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
        QStringLiteral("synopsis"), QStringLiteral("type"), QStringLiteral("status")
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
    QCOMPARE(query.value(0).toInt(), 1);
}

QTEST_MAIN(SqliteDatabaseTests)
#include "SqliteDatabaseTests.moc"
