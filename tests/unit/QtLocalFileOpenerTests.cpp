#include <QTemporaryFile>
#include <QtTest>

#include "../../src/infrastructure/library/QtLocalFileOpener.h"

class QtLocalFileOpenerTests final : public QObject {
    Q_OBJECT
private slots:
    void opensExistingFileAsLocalUrl();
    void rejectsMissingFileWithoutOpening();
};

void QtLocalFileOpenerTests::opensExistingFileAsLocalUrl() {
    QTemporaryFile file; QVERIFY(file.open());
    QUrl opened;
    QtLocalFileOpener opener([&opened](const QUrl &url) { opened = url; return true; });
    QString error;
    QVERIFY2(opener.open(file.fileName(), error), qPrintable(error));
    QVERIFY(opened.isLocalFile()); QCOMPARE(opened.toLocalFile(), file.fileName());
}

void QtLocalFileOpenerTests::rejectsMissingFileWithoutOpening() {
    bool called = false;
    QtLocalFileOpener opener([&called](const QUrl &) { called = true; return true; });
    QString error;
    QVERIFY(!opener.open(QStringLiteral("Q:/missing-file.mkv"), error));
    QVERIFY(!error.isEmpty()); QVERIFY(!called);
}

QTEST_MAIN(QtLocalFileOpenerTests)
#include "QtLocalFileOpenerTests.moc"
