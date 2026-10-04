#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/database/SqliteDatabase.h"
#include "../../src/infrastructure/database/SqliteSyncTaskStateRepository.h"

namespace {
const auto ReadSql = QStringLiteral(":/sqlite/queries/read-sync-task-states.sql");
const auto UpsertSql = QStringLiteral(":/sqlite/queries/upsert-sync-task-state.sql");
const auto DeleteSql = QStringLiteral(":/sqlite/queries/delete-sync-task-state.sql");

SyncTaskState State() {
    SyncTaskState state;
    state.kind = SyncTaskKind::ActiveCatalog;
    state.partition = SyncPartition::ActiveCatalog;
    state.status = SyncTaskStatus::RetryScheduled;
    state.cacheValidity = CacheValidity::Stale;
    state.lastSucceededAt = QDateTime::fromString(QStringLiteral("2026-10-04T10:30:00Z"), Qt::ISODate);
    state.lastAttemptedAt = QDateTime::fromString(QStringLiteral("2026-10-04T11:00:00Z"), Qt::ISODate);
    state.nextRunAt = QDateTime::fromString(QStringLiteral("2026-10-04T11:15:00Z"), Qt::ISODate);
    state.lastErrorCategory = AniListSyncErrorCategory::RateLimit;
    state.safeErrorDetail = QStringLiteral("Rate limit reached; retry later.");
    state.confirmedPage = 7;
    state.confirmedCursor = QStringLiteral("page-7-cursor");
    state.priority = 25;
    state.generation = 42;
    state.consecutiveFailures = 3;
    state.consecutiveImmediateRetries = 1;
    return state;
}
}

class SqliteSyncTaskStateRepositoryTests final : public QObject {
    Q_OBJECT

private slots:
    void migrationCreatesVersionNineteenWithoutDataLoss();
    void roundTripsUtcTimestampsAndNullableValues();
    void upsertKeepsOneRowPerTaskAndPartition();
    void invalidUpsertDoesNotReplacePersistedState();
    void preservesQueryLoaderDiagnostic();
    void rejectsUnknownPersistedEnumTextAndMalformedTimestamp();
    void removesTaskPartitionState();
};

void SqliteSyncTaskStateRepositoryTests::migrationCreatesVersionNineteenWithoutDataLoss() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral("CREATE TABLE schema_version (version INTEGER PRIMARY KEY)")));
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO schema_version (version) VALUES "
        "(1),(2),(3),(4),(5),(6),(7),(8),(9),(10),(11),(12),(13),(14),(15),(16),(17),(18)")));
    QVERIFY(query.exec(QStringLiteral(
        "CREATE TABLE media (id INTEGER PRIMARY KEY, name TEXT NOT NULL, english_name TEXT, original_name TEXT, "
        "alternative_names TEXT NOT NULL DEFAULT '[]', total_chapters INTEGER NOT NULL DEFAULT 0, "
        "consumed_chapters INTEGER NOT NULL DEFAULT 0, next_chapter INTEGER NOT NULL DEFAULT 0, "
        "average_score INTEGER NOT NULL DEFAULT 0, personal_score INTEGER NOT NULL DEFAULT 0, cover_url TEXT, "
        "cover_medium_url TEXT, cover_large_url TEXT, cover_extra_large_url TEXT, synopsis TEXT, "
        "type INTEGER NOT NULL, status INTEGER NOT NULL, user_list_status INTEGER NOT NULL DEFAULT -1, "
        "local_path TEXT NOT NULL DEFAULT '', source_removed_at TEXT, season TEXT, season_year INTEGER, "
        "next_airing_episode INTEGER, next_airing_at INTEGER, anilist_url TEXT, external_links TEXT NOT NULL DEFAULT '[]')")));
    QVERIFY(query.exec(QStringLiteral("INSERT INTO media (id, name, type, status) VALUES (42, 'Preserved', 1, 1)")));

    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM schema_version WHERE version = 19")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
    QVERIFY(query.exec(QStringLiteral("SELECT name FROM media WHERE id = 42")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("Preserved"));
}

void SqliteSyncTaskStateRepositoryTests::roundTripsUtcTimestampsAndNullableValues() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    SqliteSyncTaskStateRepository repository(database.connection(), ReadSql, UpsertSql, DeleteSql);
    QString error;
    const auto expected = State();

    QVERIFY2(repository.Upsert(expected, error), qPrintable(error));
    SyncTaskState nullable = State();
    nullable.kind = SyncTaskKind::Cover;
    nullable.partition = SyncPartition::Covers;
    nullable.lastSucceededAt.reset();
    nullable.lastAttemptedAt.reset();
    nullable.nextRunAt.reset();
    nullable.lastErrorCategory = AniListSyncErrorCategory::None;
    nullable.safeErrorDetail.clear();
    nullable.confirmedPage.reset();
    nullable.confirmedCursor.reset();
    nullable.priority = 0;
    nullable.generation = 0;
    nullable.consecutiveFailures = 0;
    nullable.consecutiveImmediateRetries = 0;
    QVERIFY2(repository.Upsert(nullable, error), qPrintable(error));

    QList<SyncTaskState> states;
    QVERIFY2(repository.ReadAll(states, error), qPrintable(error));
    QCOMPARE(states.size(), 2);
    QCOMPARE(states.at(0).lastSucceededAt->toUTC(), expected.lastSucceededAt->toUTC());
    QCOMPARE(states.at(0).lastAttemptedAt->toUTC(), expected.lastAttemptedAt->toUTC());
    QCOMPARE(states.at(0).nextRunAt->toUTC(), expected.nextRunAt->toUTC());
    QCOMPARE(states.at(0).status, expected.status);
    QCOMPARE(states.at(0).safeErrorDetail, expected.safeErrorDetail);
    QCOMPARE(states.at(0).confirmedPage, expected.confirmedPage);
    QCOMPARE(states.at(0).confirmedCursor, expected.confirmedCursor);
    QCOMPARE(states.at(0).priority, expected.priority);
    QCOMPARE(states.at(0).generation, expected.generation);
    QVERIFY(!states.at(1).lastSucceededAt.has_value());
    QVERIFY(!states.at(1).lastAttemptedAt.has_value());
    QVERIFY(!states.at(1).nextRunAt.has_value());
    QVERIFY(!states.at(1).confirmedPage.has_value());
    QVERIFY(!states.at(1).confirmedCursor.has_value());
    QCOMPARE(states.at(0).consecutiveFailures, 3);
}

void SqliteSyncTaskStateRepositoryTests::upsertKeepsOneRowPerTaskAndPartition() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    SqliteSyncTaskStateRepository repository(database.connection(), ReadSql, UpsertSql, DeleteSql);
    QString error;
    auto state = State();
    QVERIFY2(repository.Upsert(state, error), qPrintable(error));
    state.consecutiveFailures = 4;
    state.cacheValidity = CacheValidity::Expired;
    QVERIFY2(repository.Upsert(state, error), qPrintable(error));

    QList<SyncTaskState> states;
    QVERIFY2(repository.ReadAll(states, error), qPrintable(error));
    QCOMPARE(states.size(), 1);
    QCOMPARE(states.first().consecutiveFailures, 4);
    QCOMPARE(states.first().cacheValidity, CacheValidity::Expired);
}

void SqliteSyncTaskStateRepositoryTests::invalidUpsertDoesNotReplacePersistedState() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    SqliteSyncTaskStateRepository repository(database.connection(), ReadSql, UpsertSql, DeleteSql);
    QString error;
    const auto original = State();
    QVERIFY2(repository.Upsert(original, error), qPrintable(error));
    auto invalid = original;
    invalid.consecutiveFailures = -1;

    QVERIFY(!repository.Upsert(invalid, error));
    QVERIFY(!error.isEmpty());
    QList<SyncTaskState> states;
    QVERIFY2(repository.ReadAll(states, error), qPrintable(error));
    QCOMPARE(states.size(), 1);
    QCOMPARE(states.first().consecutiveFailures, original.consecutiveFailures);
}

void SqliteSyncTaskStateRepositoryTests::preservesQueryLoaderDiagnostic() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    SqliteSyncTaskStateRepository repository(database.connection(), ReadSql,
                                              QStringLiteral(":/sqlite/queries/missing.sql"), DeleteSql);
    QString error;

    QVERIFY(!repository.Upsert(State(), error));
    QVERIFY(error.contains(QStringLiteral("Could not open SQL query file")));
}

void SqliteSyncTaskStateRepositoryTests::rejectsUnknownPersistedEnumTextAndMalformedTimestamp() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    SqliteSyncTaskStateRepository repository(database.connection(), ReadSql, UpsertSql, DeleteSql);
    QString error;
    QVERIFY2(repository.Upsert(State(), error), qPrintable(error));
    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral("INSERT INTO sync_task_state "
                                      "(task_kind, partition, cache_validity, last_error_category, consecutive_failures, consecutive_immediate_retries) "
                                      "VALUES ('future-task', 'covers', 'fresh', 'none', 0, 0)")));

    QList<SyncTaskState> states;
    QVERIFY2(repository.ReadAll(states, error), qPrintable(error));
    QCOMPARE(states.size(), 1);
    QCOMPARE(states.first().kind, SyncTaskKind::ActiveCatalog);
    QVERIFY(query.exec(QStringLiteral("UPDATE sync_task_state SET last_attempted_at = 'not-a-timestamp' "
                                      "WHERE task_kind = 'active-catalog'")));
    QVERIFY2(repository.ReadAll(states, error), qPrintable(error));
    QVERIFY(states.isEmpty());
}

void SqliteSyncTaskStateRepositoryTests::removesTaskPartitionState() {
    QTemporaryDir directory;
    SqliteDatabase database(directory.filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database.open());
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    SqliteSyncTaskStateRepository repository(database.connection(), ReadSql, UpsertSql, DeleteSql);
    QString error;
    QVERIFY2(repository.Upsert(State(), error), qPrintable(error));
    QVERIFY2(repository.Remove(SyncPartition::ActiveCatalog, error), qPrintable(error));

    QList<SyncTaskState> states;
    QVERIFY2(repository.ReadAll(states, error), qPrintable(error));
    QVERIFY(states.isEmpty());
}

QTEST_MAIN(SqliteSyncTaskStateRepositoryTests)
#include "SqliteSyncTaskStateRepositoryTests.moc"
