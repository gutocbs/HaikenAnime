#include <QTemporaryDir>
#include <QtTest>
#include <QFile>
#include <QSqlQuery>

#include "../../src/infrastructure/database/SqliteDatabase.h"
#include "../../src/infrastructure/database/SqliteUserPreferencesRepository.h"

class SqliteUserPreferencesRepositoryTests final : public QObject {
    Q_OBJECT
private slots:
    void readsMissingAndRoundTripsReplacement();
    void rejectsInvalidPreferencesWithoutChangingStoredRow();
    void scannerPreferencesSurviveReopen();
    void homeSortKeySurvivesReopen();
    void rejectsMalformedStoredExtensions();
    void rejectsUnsupportedStoredHomeSortKey();
    void normalizesExtensionsAtPersistenceBoundary();
};

static SqliteUserPreferencesRepository Repository(SqliteDatabase &database) {
    QFile read(QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/resources/sqlite/queries/read-user-preferences.sql"));
    QFile write(QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/resources/sqlite/queries/upsert-user-preferences.sql"));
    if (!read.open(QIODevice::ReadOnly) || !write.open(QIODevice::ReadOnly)) qFatal("Cannot load preference SQL");
    return {database.connection(), QString::fromUtf8(read.readAll()), QString::fromUtf8(write.readAll())};
}

void SqliteUserPreferencesRepositoryTests::scannerPreferencesSurviveReopen() {
    QTemporaryDir directory;
    const auto path = directory.filePath("preferences.sqlite");
    UserPreferences expected;
    expected.libraryRoot = QStringLiteral("R:\\Anime");
    expected.scanExtensions = {QStringLiteral(".webm"), QStringLiteral(".mkv")};
    QString error;
    {
        SqliteDatabase database(path);
        QVERIFY(database.open());
        QVERIFY(database.migrate());
        auto repository = Repository(database);
        QVERIFY2(repository.replace(expected, error), qPrintable(error));
    }
    SqliteDatabase reopened(path);
    QVERIFY(reopened.open());
    QVERIFY(reopened.migrate());
    auto repository = Repository(reopened);
    UserPreferences loaded;
    bool found = false;
    QVERIFY2(repository.read(loaded, found, error), qPrintable(error));
    QVERIFY(found);
    QCOMPARE(loaded.libraryRoot, QStringLiteral("R:\\Anime"));
    QCOMPARE(loaded.scanExtensions, QStringList({".webm", ".mkv"}));
    QSqlQuery query(reopened.connection());
    QVERIFY(query.exec("SELECT scan_extensions FROM user_preferences"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("[\".webm\",\".mkv\"]"));
}

void SqliteUserPreferencesRepositoryTests::homeSortKeySurvivesReopen() {
    QTemporaryDir directory;
    const auto path = directory.filePath("preferences.sqlite");
    UserPreferences expected;
    expected.homeSortKey = QStringLiteral("title_desc");
    QString error;
    {
        SqliteDatabase database(path);
        QVERIFY(database.open());
        QVERIFY(database.migrate());
        auto repository = Repository(database);
        QVERIFY2(repository.replace(expected, error), qPrintable(error));
    }

    SqliteDatabase reopened(path);
    QVERIFY(reopened.open());
    QVERIFY(reopened.migrate());
    auto repository = Repository(reopened);
    UserPreferences loaded;
    bool found = false;
    QVERIFY2(repository.read(loaded, found, error), qPrintable(error));
    QVERIFY(found);
    QCOMPARE(loaded.homeSortKey, QStringLiteral("title_desc"));
}

void SqliteUserPreferencesRepositoryTests::rejectsMalformedStoredExtensions() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath("preferences.sqlite"));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    auto repository = Repository(database);
    QString error;
    QVERIFY(repository.replace(UserPreferences{}, error));
    QSqlQuery query(database.connection());
    for (const auto &invalid : {QStringLiteral("not-json"), QStringLiteral("{}"), QStringLiteral("[1]"), QStringLiteral("[]"), QStringLiteral("[\".mkv\",\".MKV\"]")}) {
        query.prepare("UPDATE user_preferences SET scan_extensions = :extensions");
        query.bindValue(":extensions", invalid);
        QVERIFY(query.exec());
        UserPreferences loaded;
        bool found = true;
        QVERIFY(!repository.read(loaded, found, error));
        QVERIFY(!found);
        QVERIFY(!error.isEmpty());
    }
}

void SqliteUserPreferencesRepositoryTests::rejectsUnsupportedStoredHomeSortKey() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath("preferences.sqlite"));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    auto repository = Repository(database);
    QString error;
    QVERIFY(repository.replace(UserPreferences{}, error));

    QSqlQuery query(database.connection());
    QVERIFY(query.exec("UPDATE user_preferences SET home_sort_key = 'remote_rank'"));

    UserPreferences loaded;
    bool found = true;
    QVERIFY(!repository.read(loaded, found, error));
    QVERIFY(!found);
    QCOMPARE(error, QStringLiteral("Home sort key is unsupported."));
}

void SqliteUserPreferencesRepositoryTests::normalizesExtensionsAtPersistenceBoundary() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath("preferences.sqlite"));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    auto repository = Repository(database);
    UserPreferences preferences;
    preferences.scanExtensions = {"WEBM", ".MKV"};
    QString error;
    QVERIFY(repository.replace(preferences, error));
    QSqlQuery query(database.connection());
    QVERIFY(query.exec("SELECT scan_extensions FROM user_preferences"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("[\".webm\",\".mkv\"]"));
    QVERIFY(query.exec("UPDATE user_preferences SET scan_extensions = '[\"WEBM\",\".MKV\"]'"));
    UserPreferences loaded;
    bool found = false;
    QVERIFY(repository.read(loaded, found, error));
    QVERIFY(found);
    QCOMPARE(loaded.scanExtensions, QStringList({".webm", ".mkv"}));
}

void SqliteUserPreferencesRepositoryTests::readsMissingAndRoundTripsReplacement() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("test.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    auto repository = Repository(database);
    UserPreferences loaded;
    bool found = true;
    QString error;
    QVERIFY(repository.read(loaded, found, error));
    QVERIFY(!found);

    UserPreferences expected;
    expected.scoreMaximum = 100;
    expected.scoreStep = 5;
    expected.coverQuality = CoverQuality::Large;
    expected.synchronizationEnabled = false;
    expected.synchronizationIntervalMs = 1800000;
    QVERIFY(repository.replace(expected, error));
    QVERIFY(repository.read(loaded, found, error));
    QVERIFY(found);
    QVERIFY(loaded == expected);
}

void SqliteUserPreferencesRepositoryTests::rejectsInvalidPreferencesWithoutChangingStoredRow() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("test.sqlite")));
    QVERIFY(database.open());
    QVERIFY(database.migrate());
    auto repository = Repository(database);
    UserPreferences expected;
    QString error;
    QVERIFY(repository.replace(expected, error));
    UserPreferences invalid = expected;
    invalid.scoreStep = 3;
    QVERIFY(!repository.replace(invalid, error));
    UserPreferences loaded;
    bool found = false;
    QVERIFY(repository.read(loaded, found, error));
    QVERIFY(found);
    QVERIFY(loaded == expected);
}

QTEST_MAIN(SqliteUserPreferencesRepositoryTests)
#include "SqliteUserPreferencesRepositoryTests.moc"
