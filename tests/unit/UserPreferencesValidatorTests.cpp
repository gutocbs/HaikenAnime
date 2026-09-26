#include <QtTest>
#include <limits>

#include "../../src/application/configuration/UserPreferencesValidator.h"

class UserPreferencesValidatorTests final : public QObject {
    Q_OBJECT

private slots:
    void acceptsSupportedPreferences();
    void rejectsInvalidPreferences_data();
    void rejectsInvalidPreferences();
};

void UserPreferencesValidatorTests::acceptsSupportedPreferences() {
    UserPreferences tenPoint;
    QVERIFY(ValidateUserPreferences(tenPoint).valid);

    UserPreferences hundredPoint;
    hundredPoint.scoreMaximum = 100.0;
    hundredPoint.scoreStep = 5.0;
    hundredPoint.coverQuality = CoverQuality::ExtraLarge;
    hundredPoint.synchronizationIntervalMs = 86400000;
    QVERIFY(ValidateUserPreferences(hundredPoint).valid);
}

void UserPreferencesValidatorTests::rejectsInvalidPreferences_data() {
    QTest::addColumn<double>("minimum");
    QTest::addColumn<double>("maximum");
    QTest::addColumn<double>("step");
    QTest::addColumn<int>("quality");
    QTest::addColumn<int>("interval");

    QTest::newRow("nan") << std::numeric_limits<double>::quiet_NaN() << 10.0 << 1.0 << 0 << 3600000;
    QTest::newRow("infinity") << 0.0 << std::numeric_limits<double>::infinity() << 1.0 << 0 << 3600000;
    QTest::newRow("equal range") << 10.0 << 10.0 << 1.0 << 0 << 3600000;
    QTest::newRow("inverted range") << 11.0 << 10.0 << 1.0 << 0 << 3600000;
    QTest::newRow("zero step") << 0.0 << 10.0 << 0.0 << 0 << 3600000;
    QTest::newRow("negative step") << 0.0 << 10.0 << -1.0 << 0 << 3600000;
    QTest::newRow("oversized step") << 0.0 << 10.0 << 11.0 << 0 << 3600000;
    QTest::newRow("non divisible") << 0.0 << 10.0 << 3.0 << 0 << 3600000;
    QTest::newRow("unsupported quality") << 0.0 << 10.0 << 1.0 << 99 << 3600000;
    QTest::newRow("interval below minimum") << 0.0 << 10.0 << 1.0 << 0 << 299999;
    QTest::newRow("interval above maximum") << 0.0 << 10.0 << 1.0 << 0 << 86400001;
}

void UserPreferencesValidatorTests::rejectsInvalidPreferences() {
    QFETCH(double, minimum);
    QFETCH(double, maximum);
    QFETCH(double, step);
    QFETCH(int, quality);
    QFETCH(int, interval);
    UserPreferences preferences;
    preferences.scoreMinimum = minimum;
    preferences.scoreMaximum = maximum;
    preferences.scoreStep = step;
    preferences.coverQuality = static_cast<CoverQuality>(quality);
    preferences.synchronizationIntervalMs = interval;

    const auto result = ValidateUserPreferences(preferences);
    QVERIFY(!result.valid);
    QVERIFY(!result.error.isEmpty());
}

QTEST_MAIN(UserPreferencesValidatorTests)
#include "UserPreferencesValidatorTests.moc"
