#include <QtTest>

#include "../../src/presentation/media/MediaPresentation.h"

class MediaPresentationTests final : public QObject {
    Q_OBJECT

private slots:
    void formatProgressShowsKnownTotal();
    void formatProgressShowsQuestionMarkForUnknownTotal();
    void formatProgressClampsNegativeConsumedToZero();
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

QTEST_MAIN(MediaPresentationTests)
#include "MediaPresentationTests.moc"
