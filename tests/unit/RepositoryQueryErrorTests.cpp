#include <QTemporaryDir>
#include <QtTest>

#include <memory>

#include "../../src/infrastructure/database/SqliteDatabase.h"
#include "../../src/infrastructure/database/SqliteLocalFileRepository.h"
#include "../../src/infrastructure/database/SqliteUserPreferencesRepository.h"

namespace {
const auto QueryFileError = QStringLiteral("Could not open SQL query file");

LocalFileObservation Observation() {
    return {.rootPath = QStringLiteral("Q:/"), .relativePath = QStringLiteral("Show/01.mkv"),
            .normalizedRelativePath = QStringLiteral("show/01.mkv"), .fileName = QStringLiteral("01.mkv"),
            .extension = QStringLiteral(".mkv"), .sizeBytes = 1,
            .modifiedAt = QDateTime::currentDateTimeUtc()};
}
}

/// This executable embeds the migrations but not the SQL queries, so every repository fails to load its
/// statement. The error must name that cause instead of an empty prepare error.
class RepositoryQueryErrorTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void preferencesReplaceReportsTheMissingQuery();
    void localFileWritesReportTheMissingQuery();

private:
    std::unique_ptr<QTemporaryDir> directory_;
    std::unique_ptr<SqliteDatabase> database_;
};

void RepositoryQueryErrorTests::init() {
    directory_ = std::make_unique<QTemporaryDir>();
    database_ = std::make_unique<SqliteDatabase>(directory_->filePath(QStringLiteral("library.sqlite")));
    QVERIFY(database_->open());
    QVERIFY2(database_->migrate(), qPrintable(database_->lastError()));
}

void RepositoryQueryErrorTests::cleanup() {
    database_.reset();
    directory_.reset();
}

void RepositoryQueryErrorTests::preferencesReplaceReportsTheMissingQuery() {
    SqliteUserPreferencesRepository repository(database_->connection());
    QString error;

    QVERIFY(!repository.replace(UserPreferences{}, error));

    QVERIFY2(error.contains(QueryFileError), qPrintable(error));
}

void RepositoryQueryErrorTests::localFileWritesReportTheMissingQuery() {
    SqliteLocalFileRepository repository(database_->connection());
    QString error;

    QVERIFY(!repository.upsertBatch(1, {Observation()}, error));
    QVERIFY2(error.contains(QueryFileError), qPrintable(error));
    QVERIFY(!repository.completeScan(1, 0, error));
    QVERIFY2(error.contains(QueryFileError), qPrintable(error));
    QVERIFY(!repository.failScan(1, LibraryScanStatus::Failed, 0, {}, error));
    QVERIFY2(error.contains(QueryFileError), qPrintable(error));
}

QTEST_MAIN(RepositoryQueryErrorTests)
#include "RepositoryQueryErrorTests.moc"
