#include <QtTest>

#include "../../src/infrastructure/database/SqliteQueryConfiguration.h"

class SqliteQueryConfigurationTests : public QObject {
    Q_OBJECT

private slots:
    void successfulLoadReplacesStateAndClearsError();
};

void SqliteQueryConfigurationTests::successfulLoadReplacesStateAndClearsError() {
    SqliteQueryConfiguration configuration;
    configuration.upsertMediaPath = QStringLiteral("stale.sql");
    QString error = QStringLiteral("stale error");

    QVERIFY(configuration.load(error));
    QCOMPARE(configuration.upsertMediaPath,
             QStringLiteral(":/sqlite/queries/upsert-media.sql"));
    QCOMPARE(configuration.readActiveMediaIdsPath,
             QStringLiteral(":/sqlite/queries/read-active-media-ids.sql"));
    QCOMPARE(configuration.markMediaSourceRemovedPath,
             QStringLiteral(":/sqlite/queries/mark-media-source-removed.sql"));
    QVERIFY(error.isEmpty());
}

QTEST_MAIN(SqliteQueryConfigurationTests)
#include "SqliteQueryConfigurationTests.moc"
