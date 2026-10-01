#include <QtTest>
#include <limits>

#include "../../src/application/configuration/UserPreferencesValidator.h"

class UserPreferencesValidatorTests final : public QObject {
    Q_OBJECT

private slots:
    void acceptsSupportedPreferences();
    void rejectsInvalidPreferences_data();
    void rejectsInvalidPreferences();
    void usesExactScannerDefaults();
    void normalizesScanExtensions();
    void rejectsInvalidScanExtensions_data();
    void rejectsInvalidScanExtensions();
    void acceptsHomeSortKeyForControllerNormalization();
    void rejectsUnsupportedCardStatusPresentation();
    void rejectsUnsupportedLanguage();
    void rejectsEmptyLibraryRoot();
    void comparesScannerPreferences();
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

void UserPreferencesValidatorTests::usesExactScannerDefaults() {
    const UserPreferences preferences;
    QCOMPARE(preferences.libraryRoot, QStringLiteral("Q:\\"));
    QCOMPARE(preferences.scanExtensions, QStringList({".mkv", ".mp4", ".avi", ".webm",
                                                    ".m4v", ".mov", ".wmv", ".ts"}));
    QVERIFY(ValidateUserPreferences(preferences).valid);
}

void UserPreferencesValidatorTests::normalizesScanExtensions() {
    QString error = QStringLiteral("stale error");
    QCOMPARE(NormalizeScanExtensions({"MKV", ".Mp4", "..AVI", " .WebM "}, error),
             QStringList({".mkv", ".mp4", ".avi", ".webm"}));
    QVERIFY(error.isEmpty());
    UserPreferences preferences;
    preferences.scanExtensions = {"MKV"};
    QVERIFY(ValidateUserPreferences(preferences).valid);
}

void UserPreferencesValidatorTests::rejectsInvalidScanExtensions_data() {
    QTest::addColumn<QStringList>("extensions");
    QTest::newRow("none selected") << QStringList{};
    QTest::newRow("empty") << QStringList{""};
    QTest::newRow("blank") << QStringList{"  "};
    QTest::newRow("only dots") << QStringList{"..."};
    QTest::newRow("normalized duplicate") << QStringList{"MKV", ".mkv"};
    QTest::newRow("multiple dots duplicate") << QStringList{".mkv", "..MKV"};
    QTest::newRow("forward separator") << QStringList{".mkv", "video/mp4"};
    QTest::newRow("backward separator") << QStringList{".mkv", "video\\mp4"};
}

void UserPreferencesValidatorTests::rejectsInvalidScanExtensions() {
    QFETCH(QStringList, extensions);
    QString error;
    QVERIFY(NormalizeScanExtensions(extensions, error).isEmpty());
    QVERIFY(!error.isEmpty());
    UserPreferences preferences;
    preferences.scanExtensions = extensions;
    const auto result = ValidateUserPreferences(preferences);
    QVERIFY(!result.valid);
    QVERIFY(!result.error.isEmpty());
}

void UserPreferencesValidatorTests::acceptsHomeSortKeyForControllerNormalization() {
    UserPreferences preferences;
    preferences.homeSortKey = QStringLiteral("remote_rank");

    const auto result = ValidateUserPreferences(preferences);

    QVERIFY(result.valid);
    QVERIFY(result.error.isEmpty());
}

void UserPreferencesValidatorTests::rejectsUnsupportedCardStatusPresentation() {
    UserPreferences preferences;
    preferences.cardStatusPresentation = static_cast<CardStatusPresentation>(99);

    const auto result = ValidateUserPreferences(preferences);

    QVERIFY(!result.valid);
    QVERIFY(!result.error.isEmpty());
}

void UserPreferencesValidatorTests::rejectsUnsupportedLanguage() {
    UserPreferences preferences;
    preferences.languageKey = QStringLiteral("obsolete");

    const auto result = ValidateUserPreferences(preferences);

    QVERIFY(!result.valid);
    QVERIFY(!result.error.isEmpty());
}

void UserPreferencesValidatorTests::rejectsEmptyLibraryRoot() {
    for (const auto &root : {QString{}, QStringLiteral("   ")}) {
        UserPreferences preferences;
        preferences.libraryRoot = root;
        const auto result = ValidateUserPreferences(preferences);
        QVERIFY(!result.valid);
        QVERIFY(!result.error.isEmpty());
    }
}

void UserPreferencesValidatorTests::comparesScannerPreferences() {
    const UserPreferences defaults;
    UserPreferences changedRoot;
    changedRoot.libraryRoot = QStringLiteral("R:\\Anime");
    QVERIFY(!(defaults == changedRoot));
    UserPreferences changedExtensions;
    changedExtensions.scanExtensions = {".mp4"};
    QVERIFY(!(defaults == changedExtensions));
    QVERIFY(defaults == UserPreferences{});
}

QTEST_MAIN(UserPreferencesValidatorTests)
#include "UserPreferencesValidatorTests.moc"
