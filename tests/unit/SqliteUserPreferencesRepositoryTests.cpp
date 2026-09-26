#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/database/SqliteDatabase.h"
#include "../../src/infrastructure/database/SqliteUserPreferencesRepository.h"

class SqliteUserPreferencesRepositoryTests final : public QObject {
    Q_OBJECT
private slots:
    void readsMissingAndRoundTripsReplacement();
    void rejectsInvalidPreferencesWithoutChangingStoredRow();
};

static SqliteUserPreferencesRepository Repository(SqliteDatabase &database) {
    return {database.connection(),
            QStringLiteral("SELECT score_minimum, score_maximum, score_step, cover_quality, synchronization_enabled, synchronization_interval_ms FROM user_preferences WHERE id = 1"),
            QStringLiteral("INSERT INTO user_preferences (id, score_minimum, score_maximum, score_step, cover_quality, synchronization_enabled, synchronization_interval_ms) VALUES (1, :score_minimum, :score_maximum, :score_step, :cover_quality, :synchronization_enabled, :synchronization_interval_ms) ON CONFLICT(id) DO UPDATE SET score_minimum=excluded.score_minimum, score_maximum=excluded.score_maximum, score_step=excluded.score_step, cover_quality=excluded.cover_quality, synchronization_enabled=excluded.synchronization_enabled, synchronization_interval_ms=excluded.synchronization_interval_ms")};
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
