#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>
#include <memory>

#include "../../src/infrastructure/database/SqliteDatabase.h"
#include "../../src/infrastructure/database/SqliteLocalFileRepository.h"

namespace {
LocalFileObservation Observation(const QString &path, const QString &root = QStringLiteral("Q:/")) {
    return {root, path, path.toLower(), path.section('/', -1), QStringLiteral(".mkv"), 1234,
            QDateTime::fromString(QStringLiteral("2026-09-26T12:00:00.123Z"), Qt::ISODateWithMs)};
}
}

class SqliteLocalFileRepositoryTests final : public QObject {
    Q_OBJECT
private slots:
    void init();
    void cleanup();
    void beginsRunningScan();
    void upsertsMultipleBatchesAndPreservesDistinctPaths();
    void completesAndReconcilesOnlyItsRoot();
    void failedAndInterruptedScansPreserveAvailability_data();
    void failedAndInterruptedScansPreserveAvailability();
    void rollsBackWholeBatchWhenLaterWriteFails();
    void rollsBackReconciliationWhenCompletionFails();
    void rejectsInvalidOrTerminalScans();
    void rejectsInvalidInput();
    void reportsStatementAndTransactionErrors();
    void rejectsIgnoredScanInsert();
    void rollsBackWhenCommitFails();
    void readsRecognitionQueueInPriorityOrder();
    void savesRecognitionBatch();
private:
    QVariant scalar(const QString &sql);
    QString statement(const QString &name);
    std::unique_ptr<QTemporaryDir> directory_;
    std::unique_ptr<SqliteDatabase> database_;
    std::unique_ptr<SqliteLocalFileRepository> repository_;
    QString error_;
};

QString SqliteLocalFileRepositoryTests::statement(const QString &name) {
    return QStringLiteral(":/sqlite/queries/%1.sql").arg(name);
}

QVariant SqliteLocalFileRepositoryTests::scalar(const QString &sql) {
    QSqlQuery query(database_->connection());
    if (!query.exec(sql) || !query.next()) qFatal("Test query failed: %s", qPrintable(query.lastError().text()));
    return query.value(0);
}

void SqliteLocalFileRepositoryTests::init() {
    directory_ = std::make_unique<QTemporaryDir>();
    database_ = std::make_unique<SqliteDatabase>(directory_->filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database_->open());
    QVERIFY2(database_->migrate(), qPrintable(database_->lastError()));
    repository_ = std::make_unique<SqliteLocalFileRepository>(database_->connection(),
        statement(QStringLiteral("begin-library-scan")), statement(QStringLiteral("upsert-local-file")),
        statement(QStringLiteral("complete-library-scan")), statement(QStringLiteral("fail-library-scan")),
        statement(QStringLiteral("mark-local-files-unavailable")),
        statement(QStringLiteral("read-pending-local-files")),
        statement(QStringLiteral("read-catalog-media-for-recognition")),
        statement(QStringLiteral("save-local-file-recognition")));
    error_.clear();
}

void SqliteLocalFileRepositoryTests::cleanup() {
    repository_.reset();
    database_.reset();
    directory_.reset();
}

void SqliteLocalFileRepositoryTests::beginsRunningScan() {
    qint64 id = -1;
    error_ = QStringLiteral("stale");
    QVERIFY2(repository_->beginScan(QStringLiteral("Q:/"), id, error_), qPrintable(error_));
    QVERIFY(id > 0);
    QVERIFY(error_.isEmpty());
    QCOMPARE(scalar(QStringLiteral("SELECT root_path FROM library_scans")).toString(), QStringLiteral("Q:/"));
    QCOMPARE(scalar(QStringLiteral("SELECT status FROM library_scans")).toString(), QStringLiteral("running"));
    QCOMPARE(scalar(QStringLiteral("SELECT observed_count FROM library_scans")).toInt(), 0);
    QVERIFY(scalar(QStringLiteral("SELECT finished_at FROM library_scans")).isNull());
    QVERIFY(QDateTime::fromString(scalar(QStringLiteral("SELECT started_at FROM library_scans")).toString(), Qt::ISODateWithMs).isValid());
}

void SqliteLocalFileRepositoryTests::upsertsMultipleBatchesAndPreservesDistinctPaths() {
    qint64 id;
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), id, error_));
    QVERIFY2(repository_->upsertBatch(id, {Observation(QStringLiteral("Show/Episode.MKV")),
                                          Observation(QStringLiteral("Copy/Episode.MKV"))}, error_), qPrintable(error_));
    QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM local_files WHERE size_bytes = 1234 AND file_name = 'Episode.MKV'")).toInt(), 2);
    const auto originalId = scalar(QStringLiteral("SELECT id FROM local_files WHERE normalized_relative_path = 'show/episode.mkv'")).toLongLong();
    QSqlQuery query(database_->connection());
    QVERIFY(query.exec(QStringLiteral("UPDATE local_files SET recognition_state = 'associated'")));
    auto changed = Observation(QStringLiteral("SHOW/EPISODE.mkv"));
    changed.sizeBytes = 4321;
    QVERIFY2(repository_->upsertBatch(id, {changed, Observation(QStringLiteral("Copy/Episode.MKV"))}, error_), qPrintable(error_));
    QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM local_files")).toInt(), 2);
    QCOMPARE(scalar(QStringLiteral("SELECT id FROM local_files WHERE normalized_relative_path = 'show/episode.mkv'")).toLongLong(), originalId);
    QCOMPARE(scalar(QStringLiteral("SELECT relative_path FROM local_files WHERE id = %1").arg(originalId)).toString(), QStringLiteral("SHOW/EPISODE.mkv"));
    QCOMPARE(scalar(QStringLiteral("SELECT size_bytes FROM local_files WHERE id = %1").arg(originalId)).toLongLong(), 4321);
    QCOMPARE(scalar(QStringLiteral("SELECT recognition_state FROM local_files WHERE id = %1").arg(originalId)).toString(), QStringLiteral("unprocessed"));
    QCOMPARE(scalar(QStringLiteral("SELECT modified_at FROM local_files WHERE id = %1").arg(originalId)).toString(), QStringLiteral("2026-09-26T12:00:00.123Z"));
    QCOMPARE(scalar(QStringLiteral("SELECT file_name FROM local_files WHERE id = %1").arg(originalId)).toString(), QStringLiteral("EPISODE.mkv"));
    QVERIFY(repository_->upsertBatch(id, {}, error_));
}

void SqliteLocalFileRepositoryTests::readsRecognitionQueueInPriorityOrder() {
    qint64 scanId = 0;
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), scanId, error_));
    QVERIFY(repository_->upsertBatch(scanId, {Observation(QStringLiteral("zeta.mkv")),
                                               Observation(QStringLiteral("alpha.mkv")),
                                               Observation(QStringLiteral("beta.mkv"))}, error_));
    QVERIFY(repository_->completeScan(scanId, 3, error_));
    QSqlQuery query(database_->connection());
    QVERIFY(query.exec(QStringLiteral("UPDATE local_files SET recognition_state = CASE file_name "
                                     "WHEN 'zeta.mkv' THEN 'unrecognized' "
                                     "WHEN 'alpha.mkv' THEN 'unprocessed' ELSE 'ambiguous' END")));

    QList<LocalFileRecognitionRecord> records;
    QVERIFY2(repository_->readPendingRecognition(QStringLiteral("Q:/"), records, error_), qPrintable(error_));
    QCOMPARE(records.size(), 3);
    QCOMPARE(records.at(0).normalizedRelativePath, QStringLiteral("alpha.mkv"));
    QCOMPARE(records.at(1).normalizedRelativePath, QStringLiteral("beta.mkv"));
    QCOMPARE(records.at(2).normalizedRelativePath, QStringLiteral("zeta.mkv"));
}

void SqliteLocalFileRepositoryTests::savesRecognitionBatch() {
    qint64 scanId = 0;
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), scanId, error_));
    QVERIFY(repository_->upsertBatch(scanId, {Observation(QStringLiteral("episode.mkv"))}, error_));
    QVERIFY(repository_->completeScan(scanId, 1, error_));
    const auto id = scalar(QStringLiteral("SELECT id FROM local_files")).toLongLong();
    QSqlQuery media(database_->connection());
    QVERIFY(media.exec(QStringLiteral(
        "INSERT INTO media (id, name, alternative_names, type, status) "
        "VALUES (42, 'Example', '[]', 0, 0)")));
    LocalFileRecognitionRecord record;
    record.id = id;
    record.recognitionState = QStringLiteral("associated");
    record.extractedTitle = QStringLiteral("Example");
    record.mediaKind = QStringLiteral("anime");
    record.season = 1;
    record.episode = 3;
    record.mediaId = 42;
    record.diagnostic = QStringLiteral("matched");
    QVERIFY2(repository_->saveRecognitionBatch({record}, error_), qPrintable(error_));
    QCOMPARE(scalar(QStringLiteral("SELECT recognition_state FROM local_files")).toString(), QStringLiteral("associated"));
    QCOMPARE(scalar(QStringLiteral("SELECT extracted_title FROM local_files")).toString(), QStringLiteral("Example"));
    QCOMPARE(scalar(QStringLiteral("SELECT episode FROM local_files")).toInt(), 3);
    QCOMPARE(scalar(QStringLiteral("SELECT media_id FROM local_files")).toLongLong(), 42);
}

void SqliteLocalFileRepositoryTests::completesAndReconcilesOnlyItsRoot() {
    qint64 initial, other, next;
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), initial, error_));
    QVERIFY(repository_->upsertBatch(initial, {Observation(QStringLiteral("Keep.mkv")), Observation(QStringLiteral("Gone.mkv"))}, error_));
    QVERIFY(repository_->completeScan(initial, 2, error_));
    QVERIFY(repository_->beginScan(QStringLiteral("R:/"), other, error_));
    QVERIFY(repository_->upsertBatch(other, {Observation(QStringLiteral("Gone.mkv"), QStringLiteral("R:/"))}, error_));
    QVERIFY(repository_->completeScan(other, 1, error_));
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), next, error_));
    QVERIFY(repository_->upsertBatch(next, {Observation(QStringLiteral("Keep.mkv"))}, error_));
    QVERIFY2(repository_->completeScan(next, 1, error_), qPrintable(error_));
    QCOMPARE(scalar(QStringLiteral("SELECT available FROM local_files WHERE root_path = 'Q:/' AND normalized_relative_path = 'gone.mkv'")).toInt(), 0);
    QCOMPARE(scalar(QStringLiteral("SELECT available FROM local_files WHERE root_path = 'R:/'")).toInt(), 1);
    QCOMPARE(scalar(QStringLiteral("SELECT last_seen_scan_id FROM local_files WHERE normalized_relative_path = 'keep.mkv'")).toLongLong(), next);
    QCOMPARE(scalar(QStringLiteral("SELECT status FROM library_scans WHERE id = %1").arg(next)).toString(), QStringLiteral("succeeded"));
    QCOMPARE(scalar(QStringLiteral("SELECT observed_count FROM library_scans WHERE id = %1").arg(next)).toInt(), 1);
    QVERIFY(!scalar(QStringLiteral("SELECT finished_at FROM library_scans WHERE id = %1").arg(next)).isNull());
    qint64 reappeared;
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), reappeared, error_));
    QVERIFY(repository_->upsertBatch(reappeared, {Observation(QStringLiteral("Gone.mkv"))}, error_));
    QCOMPARE(scalar(QStringLiteral("SELECT available FROM local_files WHERE root_path = 'Q:/' AND normalized_relative_path = 'gone.mkv'")).toInt(), 1);
    qint64 empty;
    QVERIFY(repository_->completeScan(reappeared, 1, error_));
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), empty, error_));
    QVERIFY(repository_->completeScan(empty, 0, error_));
    QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM local_files WHERE root_path = 'Q:/' AND available = 1")).toInt(), 0);
}

void SqliteLocalFileRepositoryTests::failedAndInterruptedScansPreserveAvailability_data() {
    QTest::addColumn<bool>("interrupted");
    QTest::newRow("failed") << false;
    QTest::newRow("interrupted") << true;
}

void SqliteLocalFileRepositoryTests::failedAndInterruptedScansPreserveAvailability() {
    QFETCH(bool, interrupted);
    qint64 old, current;
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), old, error_));
    QVERIFY(repository_->upsertBatch(old, {Observation(QStringLiteral("Old.mkv"))}, error_));
    QVERIFY(repository_->completeScan(old, 1, error_));
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), current, error_));
    QVERIFY(repository_->upsertBatch(current, {Observation(QStringLiteral("New.mkv"))}, error_));
    QVERIFY(repository_->failScan(current, interrupted ? LibraryScanStatus::Interrupted : LibraryScanStatus::Failed,
                                  1, QStringLiteral("Traversal stopped"), error_));
    QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM local_files WHERE available = 1")).toInt(), 2);
    QCOMPARE(scalar(QStringLiteral("SELECT status FROM library_scans WHERE id = %1").arg(current)).toString(),
             interrupted ? QStringLiteral("interrupted") : QStringLiteral("failed"));
    QCOMPARE(scalar(QStringLiteral("SELECT diagnostic FROM library_scans WHERE id = %1").arg(current)).toString(), QStringLiteral("Traversal stopped"));
    QCOMPARE(scalar(QStringLiteral("SELECT observed_count FROM library_scans WHERE id = %1").arg(current)).toInt(), 1);
    QVERIFY(!scalar(QStringLiteral("SELECT finished_at FROM library_scans WHERE id = %1").arg(current)).isNull());
}

void SqliteLocalFileRepositoryTests::rollsBackWholeBatchWhenLaterWriteFails() {
    qint64 id;
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), id, error_));
    QVERIFY(repository_->upsertBatch(id, {Observation(QStringLiteral("Existing.mkv"))}, error_));
    QSqlQuery query(database_->connection());
    QVERIFY(query.exec(QStringLiteral("CREATE TRIGGER reject_file BEFORE INSERT ON local_files WHEN NEW.file_name = 'Bad.mkv' BEGIN SELECT RAISE(ABORT, 'rejected'); END")));
    auto changed = Observation(QStringLiteral("Existing.mkv"));
    changed.sizeBytes = 9999;
    QVERIFY(!repository_->upsertBatch(id, {changed, Observation(QStringLiteral("Good.mkv")), Observation(QStringLiteral("Bad.mkv"))}, error_));
    QVERIFY(!error_.isEmpty());
    QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM local_files")).toInt(), 1);
    QCOMPARE(scalar(QStringLiteral("SELECT size_bytes FROM local_files")).toLongLong(), 1234);
    QVERIFY(repository_->upsertBatch(id, {Observation(QStringLiteral("After.mkv"))}, error_));
    QVERIFY(error_.isEmpty());
}

void SqliteLocalFileRepositoryTests::rollsBackReconciliationWhenCompletionFails() {
    qint64 old, current;
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), old, error_));
    QVERIFY(repository_->upsertBatch(old, {Observation(QStringLiteral("Existing.mkv"))}, error_));
    QVERIFY(repository_->completeScan(old, 1, error_));
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), current, error_));
    QSqlQuery query(database_->connection());
    QVERIFY(query.exec(QStringLiteral("CREATE TRIGGER reject_completion BEFORE UPDATE ON library_scans WHEN NEW.status = 'succeeded' BEGIN SELECT RAISE(ABORT, 'rejected'); END")));
    QVERIFY(!repository_->completeScan(current, 0, error_));
    QVERIFY(!error_.isEmpty());
    QCOMPARE(scalar(QStringLiteral("SELECT available FROM local_files")).toInt(), 1);
    QCOMPARE(scalar(QStringLiteral("SELECT status FROM library_scans WHERE id = %1").arg(current)).toString(), QStringLiteral("running"));
    QVERIFY(repository_->failScan(current, LibraryScanStatus::Failed, 0, error_, error_));
}

void SqliteLocalFileRepositoryTests::rejectsInvalidOrTerminalScans() {
    qint64 id;
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), id, error_));
    QVERIFY(!repository_->upsertBatch(id, {Observation(QStringLiteral("Wrong.mkv"), QStringLiteral("R:/"))}, error_));
    QVERIFY(!error_.isEmpty());
    QVERIFY(!repository_->upsertBatch(9999, {Observation(QStringLiteral("Unknown.mkv"))}, error_));
    QVERIFY(!repository_->completeScan(9999, 0, error_));
    QVERIFY(!repository_->failScan(id, LibraryScanStatus::Running, 0, {}, error_));
    QVERIFY(!repository_->failScan(id, LibraryScanStatus::Succeeded, 0, {}, error_));
    QVERIFY(repository_->failScan(id, LibraryScanStatus::Interrupted, 0, {}, error_));
    QVERIFY(!repository_->upsertBatch(id, {Observation(QStringLiteral("Late.mkv"))}, error_));
    QVERIFY(!repository_->completeScan(id, 0, error_));
    QVERIFY(!repository_->failScan(id, LibraryScanStatus::Failed, 0, {}, error_));
    QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM local_files")).toInt(), 0);
    QCOMPARE(scalar(QStringLiteral("SELECT status FROM library_scans")).toString(), QStringLiteral("interrupted"));
}

void SqliteLocalFileRepositoryTests::rejectsInvalidInput() {
    qint64 id = 17;
    QVERIFY(!repository_->beginScan(QStringLiteral("  "), id, error_));
    QCOMPARE(id, 0);
    QVERIFY(!error_.isEmpty());
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), id, error_));
    for (int invalid = 0; invalid < 6; ++invalid) {
        auto observation = Observation(QStringLiteral("Episode.mkv"));
        switch (invalid) {
        case 0: observation.relativePath.clear(); break;
        case 1: observation.normalizedRelativePath.clear(); break;
        case 2: observation.fileName.clear(); break;
        case 3: observation.extension.clear(); break;
        case 4: observation.sizeBytes = -1; break;
        case 5: observation.modifiedAt = {}; break;
        }
        QVERIFY(!repository_->upsertBatch(id, {observation}, error_));
        QVERIFY(!error_.isEmpty());
    }
    QVERIFY(!repository_->completeScan(id, -1, error_));
    QVERIFY(!repository_->failScan(id, LibraryScanStatus::Failed, -1, {}, error_));
    QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM local_files")).toInt(), 0);
}

void SqliteLocalFileRepositoryTests::reportsStatementAndTransactionErrors() {
    SqliteLocalFileRepository broken(database_->connection(), QStringLiteral(":/sqlite/queries/missing.sql"), {}, {}, {}, {});
    qint64 id = 17;
    QVERIFY(!broken.beginScan(QStringLiteral("Q:/"), id, error_));
    QCOMPARE(id, 0);
    QVERIFY(!error_.isEmpty());
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), id, error_));
    QVERIFY(database_->connection().transaction());
    QVERIFY(!repository_->upsertBatch(id, {Observation(QStringLiteral("Episode.mkv"))}, error_));
    QVERIFY(!error_.isEmpty());
    QVERIFY(database_->connection().rollback());
    QVERIFY(repository_->upsertBatch(id, {Observation(QStringLiteral("Episode.mkv"))}, error_));
}

void SqliteLocalFileRepositoryTests::rejectsIgnoredScanInsert() {
    qint64 first, ignored = 42;
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), first, error_));
    QSqlQuery query(database_->connection());
    QVERIFY(query.exec(QStringLiteral("CREATE TRIGGER ignore_scan BEFORE INSERT ON library_scans BEGIN SELECT RAISE(IGNORE); END")));
    QVERIFY(!repository_->beginScan(QStringLiteral("R:/"), ignored, error_));
    QCOMPARE(ignored, 0);
    QVERIFY(!error_.isEmpty());
    QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM library_scans")).toInt(), 1);
}

void SqliteLocalFileRepositoryTests::rollsBackWhenCommitFails() {
    qint64 id;
    QVERIFY(repository_->beginScan(QStringLiteral("Q:/"), id, error_));
    QSqlQuery query(database_->connection());
    QVERIFY(query.exec(QStringLiteral("CREATE TRIGGER invalidate_scan AFTER INSERT ON local_files "
                                      "BEGIN UPDATE local_files SET last_seen_scan_id = 99999 WHERE id = NEW.id; END")));
    // The invalid foreign key is accepted by the INSERT but fails at COMMIT.
    QVERIFY(query.exec(QStringLiteral("PRAGMA defer_foreign_keys = ON")));
    QVERIFY(!repository_->upsertBatch(id, {Observation(QStringLiteral("Episode.mkv"))}, error_));
    QVERIFY(!error_.isEmpty());
    QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM local_files")).toInt(), 0);
    QVERIFY(query.exec(QStringLiteral("DROP TRIGGER invalidate_scan")));
    QVERIFY(repository_->upsertBatch(id, {Observation(QStringLiteral("After.mkv"))}, error_));
}

QTEST_MAIN(SqliteLocalFileRepositoryTests)
#include "SqliteLocalFileRepositoryTests.moc"
