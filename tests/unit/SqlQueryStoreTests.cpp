#include <QDate>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/database/SqlQueryStore.h"
#include "../../src/infrastructure/logging/AsyncLogger.h"

class SqlQueryStoreTests : public QObject {
    Q_OBJECT

private slots:
    void successfulLoadReplacesOutputsAndClearsError();
    void failedLoadClearsPreviousQuery();
    void loadSourceReadsFileOnlyWhenRequested();
    void loadSourceRejectsInlineSql();
    void resourceQueryIsReadAndLoggedOncePerProcess();
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

void SqlQueryStoreTests::loadSourceReadsFileOnlyWhenRequested() {
    QTemporaryDir temporaryDirectory;
    const auto path = temporaryDirectory.filePath(QStringLiteral("query.sql"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(file.write("SELECT 2;") > 0);
    file.close();

    QString query;
    QString error;
    QVERIFY(SqlQueryStore::loadSource(path, query, error));
    QCOMPARE(query, QStringLiteral("SELECT 2;"));
    QVERIFY(error.isEmpty());
}

void SqlQueryStoreTests::loadSourceRejectsInlineSql() {
    QString query = QStringLiteral("stale query");
    QString error;

    QVERIFY(!SqlQueryStore::loadSource(QStringLiteral("SELECT 1"), query, error));
    QVERIFY(query.isEmpty());
    QVERIFY(error.contains(QStringLiteral("file"), Qt::CaseInsensitive));
}

void SqlQueryStoreTests::resourceQueryIsReadAndLoggedOncePerProcess() {
    QStandardPaths::setTestModeEnabled(true);
    const auto logPath = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(
        QStringLiteral("logs/haikenanime-%1.log").arg(QDate::currentDate().toString(Qt::ISODate)));
    QFile::remove(logPath);
    const auto source = QStringLiteral(":/sqlite/queries/read-media.sql");
    AsyncLogger logger(1);
    logger.start();
    QString first;
    QString second;
    QString error;

    QVERIFY2(SqlQueryStore::loadSource(source, first, error, &logger), qPrintable(error));
    QVERIFY2(SqlQueryStore::loadSource(source, second, error, &logger), qPrintable(error));
    logger.stop();

    QCOMPARE(second, first);
    QFile log(logPath);
    QVERIFY(log.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(log.readAll()).count(QStringLiteral("Loaded SQL query file: ") + source), 1);
}

QTEST_MAIN(SqlQueryStoreTests)
#include "SqlQueryStoreTests.moc"
