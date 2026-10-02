#include <QtTest>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryFile>

#include "../../src/infrastructure/database/SqliteQueryConfiguration.h"

class SqliteQueryConfigurationTests : public QObject {
    Q_OBJECT

private slots:
    void successfulLoadReplacesStateAndClearsError();
    void packagedInventoryQueriesAreUsable();
    void rejectsEachMissingInventoryQuery_data();
    void rejectsEachMissingInventoryQuery();
};

void SqliteQueryConfigurationTests::successfulLoadReplacesStateAndClearsError() {
    SqliteQueryConfiguration configuration;
    configuration.upsertMediaPath = QStringLiteral("stale.sql");
    QString error = QStringLiteral("stale error");

    QVERIFY(configuration.load(error));
    QCOMPARE(configuration.upsertMediaPath,
             QStringLiteral(":/sqlite/queries/upsert-media.sql"));
    QCOMPARE(configuration.updatePersonalListMediaPath,
             QStringLiteral(":/sqlite/queries/update-personal-list-media.sql"));
    QCOMPARE(configuration.readActiveMediaIdsPath,
             QStringLiteral(":/sqlite/queries/read-active-media-ids.sql"));
    QCOMPARE(configuration.markMediaSourceRemovedPath,
             QStringLiteral(":/sqlite/queries/mark-media-source-removed.sql"));
    QCOMPARE(configuration.readUserPreferencesPath,
             QStringLiteral(":/sqlite/queries/read-user-preferences.sql"));
    QCOMPARE(configuration.upsertUserPreferencesPath,
             QStringLiteral(":/sqlite/queries/upsert-user-preferences.sql"));
    QCOMPARE(configuration.beginLibraryScanPath, QStringLiteral(":/sqlite/queries/begin-library-scan.sql"));
    QCOMPARE(configuration.upsertLocalFilePath, QStringLiteral(":/sqlite/queries/upsert-local-file.sql"));
    QCOMPARE(configuration.completeLibraryScanPath, QStringLiteral(":/sqlite/queries/complete-library-scan.sql"));
    QCOMPARE(configuration.failLibraryScanPath, QStringLiteral(":/sqlite/queries/fail-library-scan.sql"));
    QCOMPARE(configuration.markLocalFilesUnavailablePath, QStringLiteral(":/sqlite/queries/mark-local-files-unavailable.sql"));
    QVERIFY(error.isEmpty());
}

void SqliteQueryConfigurationTests::packagedInventoryQueriesAreUsable() {
    QFile configuration(QStringLiteral(":/sqlite/queries/sqlite-queries.json"));
    QVERIFY(configuration.open(QIODevice::ReadOnly));
    const auto queries = QJsonDocument::fromJson(configuration.readAll()).object().value(QStringLiteral("queries")).toObject();
    for (const auto &key : {"updatePersonalListMedia", "beginLibraryScan", "upsertLocalFile", "completeLibraryScan",
                            "failLibraryScan", "markLocalFilesUnavailable"}) {
        const auto path = queries.value(QString::fromLatin1(key)).toString();
        QVERIFY2(!path.isEmpty(), key);
        QFile statement(path);
        QVERIFY2(statement.open(QIODevice::ReadOnly), qPrintable(path));
        QVERIFY(!statement.readAll().trimmed().isEmpty());
    }
}

void SqliteQueryConfigurationTests::rejectsEachMissingInventoryQuery_data() {
    QTest::addColumn<QString>("key");
    for (const auto &key : {"updatePersonalListMedia", "beginLibraryScan", "upsertLocalFile", "completeLibraryScan",
                            "failLibraryScan", "markLocalFilesUnavailable"}) {
        QTest::newRow(key) << QString::fromLatin1(key);
    }
}

void SqliteQueryConfigurationTests::rejectsEachMissingInventoryQuery() {
    QFETCH(QString, key);
    QFile packaged(QStringLiteral(":/sqlite/queries/sqlite-queries.json"));
    QVERIFY(packaged.open(QIODevice::ReadOnly));
    auto object = QJsonDocument::fromJson(packaged.readAll()).object();
    auto queries = object.value(QStringLiteral("queries")).toObject();
    queries.remove(key);
    object.insert(QStringLiteral("queries"), queries);
    QTemporaryFile file;
    QVERIFY(file.open());
    QVERIFY(file.write(QJsonDocument(object).toJson()) > 0);
    file.flush();
    SqliteQueryConfiguration configuration;
    QString error;
    QVERIFY(!configuration.load(error, file.fileName()));
    QVERIFY(error.contains(QStringLiteral("incomplete")));
}

QTEST_MAIN(SqliteQueryConfigurationTests)
#include "SqliteQueryConfigurationTests.moc"
