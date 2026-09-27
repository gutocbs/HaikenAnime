#include "SqliteLocalFileRepository.h"

#include <QSqlError>
#include <QSqlQuery>

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
}

SqliteLocalFileRepository::SqliteLocalFileRepository(QSqlDatabase database, QString beginQuery,
    QString upsertQuery, QString completeQuery, QString failQuery, QString markUnavailableQuery)
    : database_(std::move(database)), beginQuery_(std::move(beginQuery)),
      upsertQuery_(std::move(upsertQuery)), completeQuery_(std::move(completeQuery)),
      failQuery_(std::move(failQuery)), markUnavailableQuery_(std::move(markUnavailableQuery)) {}

bool SqliteLocalFileRepository::beginScan(const QString &rootPath, qint64 &scanId, QString &error) {
    scanId = 0;
    error.clear();
    if (rootPath.trimmed().isEmpty()) {
        error = QStringLiteral("A library scan requires a root path.");
        return false;
    }
    QSqlQuery query(database_);
    if (!query.prepare(beginQuery_)) {
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
    if (!query.prepare(upsertQuery_)) {
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
    if (!unavailable.prepare(markUnavailableQuery_)) {
        error = unavailable.lastError().text();
        return false;
    }
    unavailable.bindValue(QStringLiteral(":scan_id"), scanId);
    if (!unavailable.exec()) {
        error = unavailable.lastError().text();
        return false;
    }
    QSqlQuery complete(database_);
    if (!complete.prepare(completeQuery_)) {
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
    if (!query.prepare(failQuery_)) {
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
