#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/anilist/GraphQlQueryStore.h"

class GraphQlQueryStoreTests final : public QObject {
    Q_OBJECT

private slots:
    void clearsStaleOutputsBeforeLoading();
};

void GraphQlQueryStoreTests::clearsStaleOutputsBeforeLoading() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("query.graphql"));
    GraphQlQueryStore store(path);

    QString query = QStringLiteral("stale query");
    QString error = QStringLiteral("stale error");
    QVERIFY(!store.load(query, error));
    QVERIFY(query.isEmpty());
    QVERIFY(!error.isEmpty());

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    const QByteArray contents(" query MediaPage { Page { pageInfo { currentPage } } } \n");
    QCOMPARE(file.write(contents), contents.size());
    file.close();

    QVERIFY2(store.load(query, error), qPrintable(error));
    QCOMPARE(query, QStringLiteral("query MediaPage { Page { pageInfo { currentPage } } }"));
    QVERIFY(error.isEmpty());
}

QTEST_MAIN(GraphQlQueryStoreTests)
#include "GraphQlQueryStoreTests.moc"
