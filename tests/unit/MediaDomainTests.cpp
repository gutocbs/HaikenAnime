#include <QtTest>

#include "../../src/domain/media/Media.h"

class MediaDomainTests final : public QObject {
    Q_OBJECT

private slots:
    void defaultMediaHasSafeNeutralValues();
};

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
}

QTEST_MAIN(MediaDomainTests)
#include "MediaDomainTests.moc"
