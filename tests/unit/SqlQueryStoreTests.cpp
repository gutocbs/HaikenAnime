#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/database/SqlQueryStore.h"

class SqlQueryStoreTests : public QObject {
    Q_OBJECT

private slots:
    void successfulLoadReplacesOutputsAndClearsError();
    void failedLoadClearsPreviousQuery();
};

void SqlQueryStoreTests::successfulLoadReplacesOutputsAndClearsError() {
    QTemporaryDir temporaryDirectory;
    const auto path = temporaryDirectory.filePath(QStringLiteral("query.sql"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QCOMPARE(file.write(" SELECT 1; \n"), qint64(12));
    file.close();
    SqlQueryStore store(path);
    QString query = QStringLiteral("stale query");
    QString error = QStringLiteral("stale error");

    QVERIFY(store.load(query, error));
    QCOMPARE(query, QStringLiteral("SELECT 1;"));
    QVERIFY(error.isEmpty());
}

void SqlQueryStoreTests::failedLoadClearsPreviousQuery() {
    SqlQueryStore store(QStringLiteral("missing-query.sql"));
    QString query = QStringLiteral("stale query");
    QString error;

    QVERIFY(!store.load(query, error));
    QVERIFY(query.isEmpty());
    QVERIFY(!error.isEmpty());
}

QTEST_MAIN(SqlQueryStoreTests)
#include "SqlQueryStoreTests.moc"
