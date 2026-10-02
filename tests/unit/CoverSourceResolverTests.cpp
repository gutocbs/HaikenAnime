#include <QtTest>
#include "../../src/application/covers/CoverSourceResolver.h"

class CoverSourceResolverTests final : public QObject {
    Q_OBJECT
private slots:
    void resolvesRequestedOriginalAndAlternatives();
};

void CoverSourceResolverTests::resolvesRequestedOriginalAndAlternatives() {
    Media media;
    media.CoverUrl = QStringLiteral("original");
    media.CoverMediumUrl = QStringLiteral("medium");
    media.CoverLargeUrl = QStringLiteral("large");
    media.CoverExtraLargeUrl = QStringLiteral("extra");
    QCOMPARE(ResolveCoverSource(media, CoverQuality::Large), QStringLiteral("large"));

    media.CoverLargeUrl.clear();
    QCOMPARE(ResolveCoverSource(media, CoverQuality::Large), QStringLiteral("original"));
    media.CoverUrl.clear();
    QCOMPARE(ResolveCoverSource(media, CoverQuality::Large), QStringLiteral("medium"));
    media.CoverMediumUrl.clear();
    QCOMPARE(ResolveCoverSource(media, CoverQuality::Large), QStringLiteral("extra"));

    media = {};
    media.CoverLargeUrl = QStringLiteral("large");
    media.CoverExtraLargeUrl = QStringLiteral("extra");
    QCOMPARE(ResolveCoverSource(media, CoverQuality::Medium), QStringLiteral("large"));
    media = {};
    media.CoverMediumUrl = QStringLiteral("medium");
    media.CoverLargeUrl = QStringLiteral("large");
    QCOMPARE(ResolveCoverSource(media, CoverQuality::ExtraLarge), QStringLiteral("large"));
    media = {};
    QVERIFY(ResolveCoverSource(media, CoverQuality::Medium).isEmpty());
}

QTEST_MAIN(CoverSourceResolverTests)
#include "CoverSourceResolverTests.moc"
