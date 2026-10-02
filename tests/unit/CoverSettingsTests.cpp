#include <QtTest>

#include "../../src/application/covers/CoverQuality.h"

class CoverSettingsTests final : public QObject {
    Q_OBJECT

private slots:
    void parsesSupportedQuality_data();
    void parsesSupportedQuality();
    void rejectsUnsupportedQuality();
};

void CoverSettingsTests::parsesSupportedQuality_data() {
    QTest::addColumn<QString>("value");
    QTest::addColumn<CoverQuality>("expected");

    QTest::newRow("medium") << QStringLiteral("medium") << CoverQuality::Medium;
    QTest::newRow("large") << QStringLiteral("large") << CoverQuality::Large;
    QTest::newRow("extra large") << QStringLiteral("extraLarge") << CoverQuality::ExtraLarge;
}

void CoverSettingsTests::parsesSupportedQuality() {
    QFETCH(QString, value);
    QFETCH(CoverQuality, expected);

    const auto quality = ParseCoverQuality(value);

    QVERIFY(quality.has_value());
    QCOMPARE(quality.value(), expected);
    QCOMPARE(CoverQualityName(quality.value()), value);
}

void CoverSettingsTests::rejectsUnsupportedQuality() {
    QVERIFY(!ParseCoverQuality(QStringLiteral("original")).has_value());
}

QTEST_MAIN(CoverSettingsTests)
#include "CoverSettingsTests.moc"
