#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/configuration/JsonSettingsReader.h"

class JsonSettingsReaderTests : public QObject {
    Q_OBJECT

private slots:
    void readsValidConfiguration();
    void usesSafeUserPreferenceDefaultsWhenObjectIsMissing();
    void rejectsWrongUserPreferenceTypes();
    void rejectsWrongUserPreferenceSectionTypes();
    void rejectsWrongNumericTypeInsteadOfUsingDefault();
    void rejectsInvalidLibraryPreferences_data();
    void rejectsInvalidLibraryPreferences();
    void readsScannerPreferences();
    void usesScannerDefaultsForMissingSections_data();
    void usesScannerDefaultsForMissingSections();
    void readsPackagedScannerDefaults();
};

void JsonSettingsReaderTests::readsValidConfiguration() {
    QTemporaryDir temporaryDirectory;
    const auto path = temporaryDirectory.filePath(QStringLiteral("Settings.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({
        "anilist": {"endpoint": "https://graphql.anilist.co", "mediaQueryFile": "query.graphql"},
        "http": {"timeoutMs": 5000, "maxRetries": 1, "retryDelayMs": 100},
        "covers": {
            "quality": "large",
            "maxConcurrentDownloads": 2,
            "timeoutMs": 4000,
            "maxRetries": 1,
            "retryDelayMs": 50,
            "maxResponseBytes": 1048576,
            "minDimension": 24,
            "maxDimension": 2048,
            "failureCooldownMs": 60000
        },
        "sync": {"timeoutMs": 10000, "intervalMs": 60000},
        "logging": {"retentionDays": 3},
        "userPreferences": {
            "score": {"minimum": 0, "maximum": 100, "step": 5},
            "covers": {"quality": "extraLarge"},
            "sync": {"enabled": false, "intervalMs": 1800000}
        }
    })");
    file.close();
    JsonSettingsReader reader(path);
    Settings settings;
    QString error = QStringLiteral("stale error");

    QVERIFY(reader.read(settings, error));
    QCOMPARE(settings.http.timeoutMs, 5000);
    QCOMPARE(settings.http.maxRetries, 1);
    QCOMPARE(settings.covers.quality, CoverQuality::Large);
    QCOMPARE(settings.covers.maxConcurrentDownloads, 2);
    QCOMPARE(settings.covers.maxResponseBytes, 1048576);
    QCOMPARE(settings.userPreferences.scoreMinimum, 0.0);
    QCOMPARE(settings.userPreferences.scoreMaximum, 100.0);
    QCOMPARE(settings.userPreferences.scoreStep, 5.0);
    QCOMPARE(settings.userPreferences.coverQuality, CoverQuality::ExtraLarge);
    QCOMPARE(settings.userPreferences.synchronizationEnabled, false);
    QCOMPARE(settings.userPreferences.synchronizationIntervalMs, 1800000);
    QVERIFY(error.isEmpty());
}

void JsonSettingsReaderTests::usesSafeUserPreferenceDefaultsWhenObjectIsMissing() {
    QTemporaryDir temporaryDirectory;
    const auto path = temporaryDirectory.filePath(QStringLiteral("Settings.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({
        "anilist": {"endpoint": "https://graphql.anilist.co", "mediaQueryFile": "query.graphql"},
        "http": {"timeoutMs": 5000, "maxRetries": 1, "retryDelayMs": 100}
    })");
    file.close();
    JsonSettingsReader reader(path);
    Settings settings;
    QString error;

    QVERIFY(reader.read(settings, error));
    QCOMPARE(settings.userPreferences.scoreMinimum, 0.0);
    QCOMPARE(settings.userPreferences.scoreMaximum, 10.0);
    QCOMPARE(settings.userPreferences.scoreStep, 1.0);
    QCOMPARE(settings.userPreferences.coverQuality, CoverQuality::Medium);
    QCOMPARE(settings.userPreferences.synchronizationEnabled, true);
    QCOMPARE(settings.userPreferences.synchronizationIntervalMs, 3600000);
}

void JsonSettingsReaderTests::rejectsWrongUserPreferenceTypes() {
    QTemporaryDir temporaryDirectory;
    const auto path = temporaryDirectory.filePath(QStringLiteral("Settings.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({
        "anilist": {"endpoint": "https://graphql.anilist.co", "mediaQueryFile": "query.graphql"},
        "http": {"timeoutMs": 5000, "maxRetries": 1, "retryDelayMs": 100},
        "userPreferences": {
            "score": {"minimum": 0, "maximum": "ten", "step": 1},
            "covers": {"quality": "medium"},
            "sync": {"enabled": "yes", "intervalMs": 3600000}
        }
    })");
    file.close();
    JsonSettingsReader reader(path);
    Settings settings;
    QString error;

    QVERIFY(!reader.read(settings, error));
    QVERIFY(!error.isEmpty());
}

void JsonSettingsReaderTests::rejectsWrongUserPreferenceSectionTypes() {
    QTemporaryDir temporaryDirectory;
    const auto path = temporaryDirectory.filePath(QStringLiteral("Settings.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({
        "anilist": {"endpoint": "https://graphql.anilist.co", "mediaQueryFile": "query.graphql"},
        "http": {"timeoutMs": 5000, "maxRetries": 1, "retryDelayMs": 100},
        "userPreferences": {
            "score": "0-10",
            "covers": {"quality": "medium"},
            "sync": {"enabled": true, "intervalMs": 3600000}
        }
    })");
    file.close();
    JsonSettingsReader reader(path);
    Settings settings;
    QString error;

    QVERIFY(!reader.read(settings, error));
    QVERIFY(!error.isEmpty());
}

void JsonSettingsReaderTests::rejectsWrongNumericTypeInsteadOfUsingDefault() {
    QTemporaryDir temporaryDirectory;
    const auto path = temporaryDirectory.filePath(QStringLiteral("Settings.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({
        "anilist": {"endpoint": "https://graphql.anilist.co", "mediaQueryFile": "query.graphql"},
        "http": {"timeoutMs": "five seconds", "maxRetries": 1, "retryDelayMs": 100}
    })");
    file.close();
    JsonSettingsReader reader(path);
    Settings settings;
    QString error;

    QVERIFY(!reader.read(settings, error));
    QVERIFY(!error.isEmpty());
}

void JsonSettingsReaderTests::rejectsInvalidLibraryPreferences_data() {
    QTest::addColumn<QByteArray>("libraryJson");
    QTest::newRow("section string") << QByteArray(R"("Q:\\")");
    QTest::newRow("section null") << QByteArray("null");
    QTest::newRow("root number") << QByteArray(R"({"root": 123})");
    QTest::newRow("root null") << QByteArray(R"({"root": null})");
    QTest::newRow("empty root") << QByteArray(R"({"root": ""})");
    QTest::newRow("blank root") << QByteArray(R"({"root": "   "})");
    QTest::newRow("extensions string") << QByteArray(R"({"extensions": "mkv"})");
    QTest::newRow("extensions null") << QByteArray(R"({"extensions": null})");
    QTest::newRow("extensions empty") << QByteArray(R"({"extensions": []})");
    QTest::newRow("extension number") << QByteArray(R"({"extensions": [123]})");
    QTest::newRow("extension null") << QByteArray(R"({"extensions": [null]})");
    QTest::newRow("extension empty") << QByteArray(R"({"extensions": [""]})");
    QTest::newRow("extension blank") << QByteArray(R"({"extensions": ["   "]})");
    QTest::newRow("extension dot only") << QByteArray(R"({"extensions": ["..."]})");
    QTest::newRow("extension duplicate") << QByteArray(R"({"extensions": ["MKV", ".mkv"]})");
    QTest::newRow("extension forward separator") << QByteArray(R"({"extensions": ["video/mkv"]})");
    QTest::newRow("extension backward separator") << QByteArray(R"({"extensions": ["video\\mkv"]})");
}

void JsonSettingsReaderTests::rejectsInvalidLibraryPreferences() {
    QFETCH(QByteArray, libraryJson);
    QTemporaryDir temporaryDirectory;
    const auto path = temporaryDirectory.filePath(QStringLiteral("Settings.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({"anilist":{"endpoint":"https://graphql.anilist.co","mediaQueryFile":"query.graphql"},
        "http":{"timeoutMs":5000},"userPreferences":{"library":)");
    file.write(libraryJson);
    file.write("}}");
    file.close();
    JsonSettingsReader reader(path);
    Settings settings;
    QString error;
    QVERIFY(!reader.read(settings, error));
    QVERIFY(!error.isEmpty());
}

void JsonSettingsReaderTests::readsScannerPreferences() {
    QTemporaryDir temporaryDirectory;
    const auto path = temporaryDirectory.filePath(QStringLiteral("Settings.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({"anilist":{"endpoint":"https://graphql.anilist.co","mediaQueryFile":"query.graphql"},
        "http":{"timeoutMs":5000},"userPreferences":{"library":{
        "root":"R:\\Anime", "extensions":["MKV", ".Mp4", "..AVI"]}}})");
    file.close();
    JsonSettingsReader reader(path);
    Settings settings;
    QString error;
    QVERIFY2(reader.read(settings, error), qPrintable(error));
    QCOMPARE(settings.userPreferences.libraryRoot, QStringLiteral("R:\\Anime"));
    QCOMPARE(settings.userPreferences.scanExtensions, QStringList({".mkv", ".mp4", ".avi"}));
    QVERIFY(error.isEmpty());
}

void JsonSettingsReaderTests::usesScannerDefaultsForMissingSections_data() {
    QTest::addColumn<QByteArray>("preferencesJson");
    QTest::newRow("missing preferences") << QByteArray{};
    QTest::newRow("missing library") << QByteArray(R"(,"userPreferences":{})");
    QTest::newRow("empty library") << QByteArray(R"(,"userPreferences":{"library":{}})");
    QTest::newRow("missing extensions") << QByteArray(R"(,"userPreferences":{"library":{"root":"Q:\\"}})");
    QTest::newRow("missing root") << QByteArray(R"(,"userPreferences":{"library":{"extensions":[".mkv",".mp4",".avi",".webm",".m4v",".mov",".wmv",".ts"]}})");
}

void JsonSettingsReaderTests::usesScannerDefaultsForMissingSections() {
    QFETCH(QByteArray, preferencesJson);
    QTemporaryDir temporaryDirectory;
    const auto path = temporaryDirectory.filePath(QStringLiteral("Settings.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({"anilist":{"endpoint":"https://graphql.anilist.co","mediaQueryFile":"query.graphql"},"http":{"timeoutMs":5000})");
    file.write(preferencesJson);
    file.write("}");
    file.close();
    JsonSettingsReader reader(path);
    Settings settings;
    QString error;
    QVERIFY2(reader.read(settings, error), qPrintable(error));
    QCOMPARE(settings.userPreferences.libraryRoot, QStringLiteral("Q:\\"));
    QCOMPARE(settings.userPreferences.scanExtensions, QStringList({".mkv", ".mp4", ".avi", ".webm",
                                                                 ".m4v", ".mov", ".wmv", ".ts"}));
}

void JsonSettingsReaderTests::readsPackagedScannerDefaults() {
    const auto path = QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/Settings.json");
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto library = QJsonDocument::fromJson(file.readAll()).object()
                             .value("userPreferences").toObject().value("library").toObject();
    QVERIFY(library.contains("root"));
    QVERIFY(library.contains("extensions"));
    JsonSettingsReader reader(path);
    Settings settings;
    QString error;
    QVERIFY2(reader.read(settings, error), qPrintable(error));
    QCOMPARE(settings.userPreferences.libraryRoot, QStringLiteral("Q:\\"));
    QCOMPARE(settings.userPreferences.scanExtensions, QStringList({".mkv", ".mp4", ".avi", ".webm",
                                                                 ".m4v", ".mov", ".wmv", ".ts"}));
}

QTEST_MAIN(JsonSettingsReaderTests)
#include "JsonSettingsReaderTests.moc"
