#include <QtTest>

#include "../../src/application/media/ProgressFormatter.h"
#include "../../src/domain/media/Media.h"

class MediaDomainTests final : public QObject {
    Q_OBJECT

private slots:
    void defaultMediaHasSafeNeutralValues();
    void formatProgressShowsKnownTotal();
    void formatProgressShowsQuestionMarkForUnknownTotal();
    void formatProgressClampsNegativeConsumedToZero();
};

void MediaDomainTests::formatProgressShowsKnownTotal() {
    QCOMPARE(FormatProgress(3, 12), QStringLiteral("3/12"));
}

void MediaDomainTests::formatProgressShowsQuestionMarkForUnknownTotal() {
    QCOMPARE(FormatProgress(3, 0), QStringLiteral("3/?"));
    QCOMPARE(FormatProgress(0, -1), QStringLiteral("0/?"));
}

void MediaDomainTests::formatProgressClampsNegativeConsumedToZero() {
    QCOMPARE(FormatProgress(-2, 5), QStringLiteral("0/5"));
}

void MediaDomainTests::defaultMediaHasSafeNeutralValues() {
    const Media media;

    QCOMPARE(media.Id, 0);
    QCOMPARE(media.TotalChapters, 0);
    QCOMPARE(media.ConsumedChapters, 0);
    QCOMPARE(media.NextChapter, 0);
    QCOMPARE(media.AverageScore, 0);
    QCOMPARE(media.PersonalScore, 0);
    QCOMPARE(media.Type, MediaType::Unknown);
    QCOMPARE(media.Status, MediaStatus::Unknown);
    QCOMPARE(media.ListStatus, UserListStatus::Unknown);
}

QTEST_MAIN(MediaDomainTests)
#include "MediaDomainTests.moc"
