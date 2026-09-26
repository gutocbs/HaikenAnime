#include <QFile>
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

QTEST_MAIN(JsonSettingsReaderTests)
#include "JsonSettingsReaderTests.moc"
