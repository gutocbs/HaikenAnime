#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/database/SqliteDatabase.h"
#include "../../src/infrastructure/database/SqlitePluginSettingsRepository.h"

namespace {
SqlitePluginSettingsRepository repositoryFor(SqliteDatabase &database) {
    return SqlitePluginSettingsRepository(database.connection());
}
}

class SqlitePluginSettingsRepositoryTests final : public QObject {
    Q_OBJECT

private slots:
    void readsNothingForUnknownPlugin();
    void roundTripsTypedValuesAcrossReopen();
    void replaceRemovesKeysNoLongerPresent();
    void keepsPluginsIsolated();
    void migrationCreatesPluginSettingsTable();
};

void SqlitePluginSettingsRepositoryTests::readsNothingForUnknownPlugin() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("settings.sqlite")));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    auto repository = repositoryFor(database);
    QVariantMap values{{QStringLiteral("stale"), 1}};
    QString error;

    QVERIFY2(repository.read(QStringLiteral("plex"), values, error), qPrintable(error));

    QVERIFY(values.isEmpty());
}

void SqlitePluginSettingsRepositoryTests::roundTripsTypedValuesAcrossReopen() {
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("settings.sqlite"));
    const QVariantMap expected{{QStringLiteral("serverUrl"), QStringLiteral("http://nas:32400")},
                               {QStringLiteral("pollSeconds"), 30},
                               {QStringLiteral("ratio"), 0.5},
                               {QStringLiteral("enabled"), false},
                               {QStringLiteral("users"), QStringList{QStringLiteral("1"), QStringLiteral("7")}}};
    QString error;
    {
        SqliteDatabase database(path);
        QVERIFY(database.open());
        QVERIFY2(database.migrate(), qPrintable(database.lastError()));
        auto repository = repositoryFor(database);
        QVERIFY2(repository.replace(QStringLiteral("plex"), expected, error), qPrintable(error));
    }
    SqliteDatabase reopened(path);
    QVERIFY(reopened.open());
    QVERIFY2(reopened.migrate(), qPrintable(reopened.lastError()));
    auto repository = repositoryFor(reopened);
    QVariantMap loaded;

    QVERIFY2(repository.read(QStringLiteral("plex"), loaded, error), qPrintable(error));

    QCOMPARE(loaded.value(QStringLiteral("serverUrl")).toString(), QStringLiteral("http://nas:32400"));
    QCOMPARE(loaded.value(QStringLiteral("pollSeconds")).toInt(), 30);
    QCOMPARE(loaded.value(QStringLiteral("ratio")).toDouble(), 0.5);
    QCOMPARE(loaded.value(QStringLiteral("enabled")).typeId(), QMetaType::Bool);
    QCOMPARE(loaded.value(QStringLiteral("enabled")).toBool(), false);
    QCOMPARE(loaded.value(QStringLiteral("users")).toStringList(),
             QStringList({QStringLiteral("1"), QStringLiteral("7")}));
}

void SqlitePluginSettingsRepositoryTests::replaceRemovesKeysNoLongerPresent() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("settings.sqlite")));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    auto repository = repositoryFor(database);
    QString error;
    QVERIFY(repository.replace(QStringLiteral("plex"), {{QStringLiteral("a"), 1}, {QStringLiteral("b"), 2}}, error));

    QVERIFY2(repository.replace(QStringLiteral("plex"), {{QStringLiteral("b"), 3}}, error), qPrintable(error));

    QVariantMap loaded;
    QVERIFY(repository.read(QStringLiteral("plex"), loaded, error));
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded.value(QStringLiteral("b")).toInt(), 3);
}

void SqlitePluginSettingsRepositoryTests::keepsPluginsIsolated() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("settings.sqlite")));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    auto repository = repositoryFor(database);
    QString error;
    QVERIFY(repository.replace(QStringLiteral("plex"), {{QStringLiteral("pollSeconds"), 15}}, error));
    QVERIFY(repository.replace(QStringLiteral("mpv"), {{QStringLiteral("executable"), QStringLiteral("mpv.exe")}}, error));

    QVERIFY(repository.replace(QStringLiteral("mpv"), {}, error));

    QVariantMap plex;
    QVariantMap mpv;
    QVERIFY(repository.read(QStringLiteral("plex"), plex, error));
    QVERIFY(repository.read(QStringLiteral("mpv"), mpv, error));
    QCOMPARE(plex.value(QStringLiteral("pollSeconds")).toInt(), 15);
    QVERIFY(mpv.isEmpty());
}

void SqlitePluginSettingsRepositoryTests::migrationCreatesPluginSettingsTable() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("settings.sqlite")));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QSqlQuery query(database.connection());

    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM schema_version WHERE version = 24")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
    QVERIFY(query.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'plugin_setting'")));
    QVERIFY(query.next());
}

QTEST_MAIN(SqlitePluginSettingsRepositoryTests)
#include "SqlitePluginSettingsRepositoryTests.moc"
