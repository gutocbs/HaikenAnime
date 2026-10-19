#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/presentation/media/MediaPresentation.h"

class MediaPresentationTests final : public QObject {
    Q_OBJECT

private slots:
    void formatProgressShowsKnownTotal();
    void formatProgressShowsQuestionMarkForUnknownTotal();
    void formatProgressClampsNegativeConsumedToZero();
    void localFolderUrlPointsToAnExistingFolder();
    void localFolderUrlIsEmptyForAMissingFolder();
};

void MediaPresentationTests::formatProgressShowsKnownTotal() {
    QCOMPARE(FormatProgress(3, 12), QStringLiteral("3/12"));
}

void MediaPresentationTests::formatProgressShowsQuestionMarkForUnknownTotal() {
    QCOMPARE(FormatProgress(3, 0), QStringLiteral("3/?"));
    QCOMPARE(FormatProgress(0, -1), QStringLiteral("0/?"));
}

void MediaPresentationTests::formatProgressClampsNegativeConsumedToZero() {
    QCOMPARE(FormatProgress(-2, 5), QStringLiteral("0/5"));
}

void MediaPresentationTests::localFolderUrlPointsToAnExistingFolder() {
    QTemporaryDir directory;

    QCOMPARE(PresentLocalFolderUrl(QDir::toNativeSeparators(directory.path())),
             QUrl::fromLocalFile(directory.path()).toString());
}

void MediaPresentationTests::localFolderUrlIsEmptyForAMissingFolder() {
    QTemporaryDir directory;

    QVERIFY(PresentLocalFolderUrl({}).isEmpty());
    QVERIFY(PresentLocalFolderUrl(directory.filePath(QStringLiteral("missing"))).isEmpty());
}

QTEST_MAIN(MediaPresentationTests)
#include "MediaPresentationTests.moc"
