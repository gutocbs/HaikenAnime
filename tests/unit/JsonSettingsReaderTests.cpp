#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/configuration/JsonSettingsReader.h"

class JsonSettingsReaderTests : public QObject {
    Q_OBJECT

private slots:
    void readsValidConfiguration();
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
        "sync": {"timeoutMs": 10000, "intervalMs": 60000},
        "logging": {"retentionDays": 3}
    })");
    file.close();
    JsonSettingsReader reader(path);
    Settings settings;
    QString error = QStringLiteral("stale error");

    QVERIFY(reader.read(settings, error));
    QCOMPARE(settings.http.timeoutMs, 5000);
    QCOMPARE(settings.http.maxRetries, 1);
    QVERIFY(error.isEmpty());
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
