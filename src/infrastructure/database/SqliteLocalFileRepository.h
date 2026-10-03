#ifndef HAIKENANIME_SQLITELOCALFILEREPOSITORY_H
#define HAIKENANIME_SQLITELOCALFILEREPOSITORY_H

#include <QSqlDatabase>
#include "../../application/library/ILocalFileRepository.h"

// Use only on the thread that owns the injected connection. Statements are loaded
// from external SQL resources by the composition layer.
class SqliteLocalFileRepository final : public ILocalFileRepository {
public:
    SqliteLocalFileRepository(QSqlDatabase database, QString beginQuery, QString upsertQuery,
                             QString completeQuery, QString failQuery, QString markUnavailableQuery,
                             QString readPendingQuery = {}, QString readCatalogQuery = {}, QString saveRecognitionQuery = {});
    bool beginScan(const QString &rootPath, qint64 &scanId, QString &error) override;
    bool upsertBatch(qint64 scanId, const QList<LocalFileObservation> &observations, QString &error) override;
    bool completeScan(qint64 scanId, qsizetype observedCount, QString &error) override;
    bool failScan(qint64 scanId, LibraryScanStatus status, qsizetype observedCount,
                  const QString &diagnostic, QString &error) override;
    bool readPendingRecognition(const QString &rootPath, QList<LocalFileRecognitionRecord> &records,
                                QString &error) override;
    bool readRecognitionCatalog(QList<Media> &media, QString &error) override;
    bool saveRecognitionBatch(const QList<LocalFileRecognitionRecord> &records, QString &error) override;
private:
    QSqlDatabase database_;
    QString beginQuery_;
    QString upsertQuery_;
    QString completeQuery_;
    QString failQuery_;
    QString markUnavailableQuery_;
    QString readPendingQuery_;
    QString readCatalogQuery_;
    QString saveRecognitionQuery_;
};

#endif
