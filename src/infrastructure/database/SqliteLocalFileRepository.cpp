#include "SqliteLocalFileRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QPair>

#include "SqliteMediaMapper.h"
#include "../../application/library/LocalMediaCatalogIndex.h"
#include "SqliteQueries.h"
#include "SqlQueryStore.h"

#include <utility>

namespace {
class Transaction final {
public:
    explicit Transaction(QSqlDatabase &database) : database_(database) {}
    ~Transaction() { if (active_) database_.rollback(); }
    bool begin(QString &error) {
        active_ = database_.transaction();
        if (!active_) error = database_.lastError().text();
        return active_;
    }
    bool commit(QString &error) {
        if (!database_.commit()) {
            error = database_.lastError().text();
            return false;
        }
        active_ = false;
        return true;
    }
private:
    QSqlDatabase &database_;
    bool active_ = false;
};

bool ExecuteSingleRow(QSqlQuery &query, QString &error) {
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() != 1) {
        error = QStringLiteral("The library scan is not running or does not match the observation root.");
        return false;
    }
    return true;
}

LocalFileRecognitionRecord MapRecognitionRecord(const QSqlQuery &query) {
    LocalFileRecognitionRecord record;
    record.id = query.value(0).toLongLong(); record.rootPath = query.value(1).toString();
    record.relativePath = query.value(2).toString(); record.normalizedRelativePath = query.value(3).toString();
    record.fileName = query.value(4).toString(); record.sizeBytes = query.value(5).toLongLong();
    record.modifiedAt = QDateTime::fromString(query.value(6).toString(), Qt::ISODateWithMs);
    record.available = query.value(7).toBool(); record.recognitionState = query.value(8).toString();
    record.extractedTitle = query.value(9).toString(); record.mediaKind = query.value(10).toString();
    if (!query.value(11).isNull()) record.season = query.value(11).toInt();
    if (!query.value(12).isNull()) record.episode = query.value(12).toInt();
    if (!query.value(13).isNull()) record.mediaId = query.value(13).toInt();
    record.diagnostic = query.value(14).toString();
    return record;
}
}

SqliteLocalFileRepository::SqliteLocalFileRepository(QSqlDatabase database) : database_(std::move(database)) {}

bool SqliteLocalFileRepository::beginScan(const QString &rootPath, qint64 &scanId, QString &error) {
    scanId = 0;
    error.clear();
    if (rootPath.trimmed().isEmpty()) {
        error = QStringLiteral("A library scan requires a root path.");
        return false;
    }
    QSqlQuery query(database_);
    QString beginQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::BeginLibraryScan, beginQuery, error)) {
        return false;
    }
    if (!query.prepare(beginQuery)) {
        error = query.lastError().text();
        return false;
    }
    query.bindValue(QStringLiteral(":root_path"), rootPath);
    query.bindValue(QStringLiteral(":started_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() != 1 || query.lastInsertId().toLongLong() <= 0) {
        error = QStringLiteral("The library scan was not created.");
        return false;
    }
    scanId = query.lastInsertId().toLongLong();
    return true;
}
bool SqliteLocalFileRepository::upsertBatch(qint64 scanId, const QList<LocalFileObservation> &observations,
                                           QString &error) {
    error.clear();
    if (scanId <= 0) {
        error = QStringLiteral("A library batch requires a valid scan ID.");
        return false;
    }
    for (const auto &observation : observations) {
        if (observation.rootPath.trimmed().isEmpty() || observation.relativePath.isEmpty()
            || observation.normalizedRelativePath.isEmpty() || observation.fileName.isEmpty()
            || observation.extension.isEmpty() || observation.sizeBytes < 0 || !observation.modifiedAt.isValid()) {
            error = QStringLiteral("A local file observation requires paths, filename, extension, nonnegative size and modification time.");
            return false;
        }
    }
    if (observations.isEmpty()) return true;

    Transaction transaction(database_);
    if (!transaction.begin(error)) return false;
    QSqlQuery query(database_);
    QString upsertQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::UpsertLocalFile, upsertQuery, error)
        || !query.prepare(upsertQuery)) {
        error = query.lastError().text();
        return false;
    }
    for (const auto &observation : observations) {
        query.bindValue(QStringLiteral(":scan_id"), scanId);
        query.bindValue(QStringLiteral(":root_path"), observation.rootPath);
        query.bindValue(QStringLiteral(":relative_path"), observation.relativePath);
        query.bindValue(QStringLiteral(":normalized_relative_path"), observation.normalizedRelativePath);
        query.bindValue(QStringLiteral(":file_name"), observation.fileName);
        query.bindValue(QStringLiteral(":extension"), observation.extension);
        query.bindValue(QStringLiteral(":size_bytes"), observation.sizeBytes);
        query.bindValue(QStringLiteral(":modified_at"), observation.modifiedAt.toUTC().toString(Qt::ISODateWithMs));
        if (!ExecuteSingleRow(query, error)) return false;
    }
    return transaction.commit(error);
}
bool SqliteLocalFileRepository::completeScan(qint64 scanId, qsizetype observedCount, QString &error) {
    error.clear();
    if (scanId <= 0 || observedCount < 0) {
        error = QStringLiteral("Completing a library scan requires a valid ID and nonnegative count.");
        return false;
    }
    Transaction transaction(database_);
    if (!transaction.begin(error)) return false;
    QSqlQuery unavailable(database_);
    QString markUnavailableQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::MarkLocalFilesUnavailable, markUnavailableQuery, error)
        || !unavailable.prepare(markUnavailableQuery)) {
        error = unavailable.lastError().text();
        return false;
    }
    unavailable.bindValue(QStringLiteral(":scan_id"), scanId);
    if (!unavailable.exec()) {
        error = unavailable.lastError().text();
        return false;
    }
    QSqlQuery complete(database_);
    QString completeQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::CompleteLibraryScan, completeQuery, error)
        || !complete.prepare(completeQuery)) {
        error = complete.lastError().text();
        return false;
    }
    complete.bindValue(QStringLiteral(":scan_id"), scanId);
    complete.bindValue(QStringLiteral(":observed_count"), static_cast<qint64>(observedCount));
    complete.bindValue(QStringLiteral(":finished_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    if (!ExecuteSingleRow(complete, error)) return false;
    return transaction.commit(error);
}

bool SqliteLocalFileRepository::failScan(qint64 scanId, LibraryScanStatus status, qsizetype observedCount,
                                        const QString &diagnostic, QString &error) {
    const auto savedDiagnostic = diagnostic.isNull() ? QStringLiteral("") : diagnostic;
    error.clear();
    if (scanId <= 0 || observedCount < 0
        || (status != LibraryScanStatus::Failed && status != LibraryScanStatus::Interrupted)) {
        error = QStringLiteral("Failing a library scan requires a valid ID, nonnegative count and Failed or Interrupted status.");
        return false;
    }
    QSqlQuery query(database_);
    QString failQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::FailLibraryScan, failQuery, error)
        || !query.prepare(failQuery)) {
        error = query.lastError().text();
        return false;
    }
    query.bindValue(QStringLiteral(":scan_id"), scanId);
    query.bindValue(QStringLiteral(":status"), status == LibraryScanStatus::Failed
                    ? QStringLiteral("failed") : QStringLiteral("interrupted"));
    query.bindValue(QStringLiteral(":observed_count"), static_cast<qint64>(observedCount));
    query.bindValue(QStringLiteral(":finished_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    query.bindValue(QStringLiteral(":diagnostic"), savedDiagnostic);
    return ExecuteSingleRow(query, error);
}

bool SqliteLocalFileRepository::readPendingRecognition(const QString &rootPath,
                                                       QList<LocalFileRecognitionRecord> &records,
                                                       QString &error) {
    records.clear(); error.clear();
    QString readPendingQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::ReadPendingLocalFiles, readPendingQuery, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(readPendingQuery)) { error = query.lastError().text(); return false; }
    query.bindValue(QStringLiteral(":root_path"), rootPath);
    if (!query.exec()) { error = query.lastError().text(); return false; }
    while (query.next()) records.append(MapRecognitionRecord(query));
    return true;
}

bool SqliteLocalFileRepository::readFilesUnderFolder(const QString &folderPath,
                                                     QList<LocalFileRecognitionRecord> &records,
                                                     QString &error) {
    records.clear(); error.clear();
    const auto folder = LocalMediaCatalogIndex::NormalizeFolder(folderPath);
    if (folder.isEmpty()) {
        error = QStringLiteral("Reading local files under a folder requires a folder path.");
        return false;
    }
    QString readRootsQuery;
    QString readUnderFolderQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::ReadLocalFileRoots, readRootsQuery, error)
        || !SqlQueryStore::loadSource(SqliteQueries::ReadLocalFilesUnderFolder, readUnderFolderQuery, error)) return false;
    // Root and folder are compared in C++ because SQLite NOCASE only folds ASCII.
    QSqlQuery rootsQuery(database_);
    if (!rootsQuery.prepare(readRootsQuery) || !rootsQuery.exec()) { error = rootsQuery.lastError().text(); return false; }
    QList<QPair<QString, QString>> matches;
    while (rootsQuery.next()) {
        const auto rootPath = rootsQuery.value(0).toString();
        const auto root = LocalMediaCatalogIndex::NormalizeFolder(rootPath);
        if (root.isEmpty()) continue;
        if (folder == root) { matches.append({rootPath, QStringLiteral("")});  /* non-null: a null binds as SQL NULL */ continue; }
        // The separator boundary keeps "Show" from matching "Show 2".
        if (folder.startsWith(root + QLatin1Char('/'))) {
            // normalized_relative_path is stored case-folded with '/' separators by the scanner.
            matches.append({rootPath, folder.mid(root.size() + 1).toCaseFolded() + QLatin1Char('/')});
        }
    }
    QSqlQuery query(database_);
    if (!query.prepare(readUnderFolderQuery)) { error = query.lastError().text(); return false; }
    for (const auto &match : matches) {
        query.bindValue(QStringLiteral(":root_path"), match.first);
        query.bindValue(QStringLiteral(":prefix"), match.second);
        if (!query.exec()) { error = query.lastError().text(); return false; }
        while (query.next()) records.append(MapRecognitionRecord(query));
    }
    return true;
}

bool SqliteLocalFileRepository::readRecognitionCatalog(QList<Media> &media, QString &error) {
    media.clear(); error.clear();
    QString readCatalogQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::ReadCatalogMediaForRecognition, readCatalogQuery, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(readCatalogQuery) || !query.exec()) { error = query.lastError().text(); return false; }
    while (query.next()) media.append(SqliteMediaMapper::Map(query));
    return true;
}

bool SqliteLocalFileRepository::saveRecognitionBatch(const QList<LocalFileRecognitionRecord> &records,
                                                     QString &error) {
    error.clear();
    QString saveRecognitionQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::SaveLocalFileRecognition, saveRecognitionQuery, error)) return false;
    if (records.isEmpty()) return true;
    Transaction transaction(database_);
    if (!transaction.begin(error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(saveRecognitionQuery)) { error = query.lastError().text(); return false; }
    for (const auto &record : records) {
        query.bindValue(QStringLiteral(":id"), record.id);
        query.bindValue(QStringLiteral(":recognition_state"), record.recognitionState);
        query.bindValue(QStringLiteral(":extracted_title"), record.extractedTitle);
        query.bindValue(QStringLiteral(":media_kind"), record.mediaKind);
        query.bindValue(QStringLiteral(":season"), record.season ? QVariant(*record.season) : QVariant());
        query.bindValue(QStringLiteral(":episode"), record.episode ? QVariant(*record.episode) : QVariant());
        query.bindValue(QStringLiteral(":media_id"), record.mediaId ? QVariant(*record.mediaId) : QVariant());
        query.bindValue(QStringLiteral(":diagnostic"), record.diagnostic);
        query.bindValue(QStringLiteral(":recognized_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
        if (!ExecuteSingleRow(query, error)) return false;
    }
    return transaction.commit(error);
}
