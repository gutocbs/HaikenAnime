#ifndef HAIKENANIME_SQLITELOCALFILEREPOSITORY_H
#define HAIKENANIME_SQLITELOCALFILEREPOSITORY_H

#include <QSqlDatabase>
#include "../../application/library/ILocalFileRecognitionRepository.h"
#include "../../application/library/ILocalFileScanRepository.h"

// Use only on the thread that owns the injected connection. Statements come from the
// SQL resources listed in SqliteQueries. Not final: the composition derives from it to own the
// worker connection.
class SqliteLocalFileRepository : public ILocalFileScanRepository, public ILocalFileRecognitionRepository {
public:
    explicit SqliteLocalFileRepository(QSqlDatabase database);
    bool beginScan(const QString &rootPath, qint64 &scanId, QString &error) override;
    bool upsertBatch(qint64 scanId, const QList<LocalFileObservation> &observations, QString &error) override;
    bool completeScan(qint64 scanId, qsizetype observedCount, QString &error) override;
    bool failScan(qint64 scanId, LibraryScanStatus status, qsizetype observedCount,
                  const QString &diagnostic, QString &error) override;
    bool readPendingRecognition(const QString &rootPath, QList<LocalFileRecognitionRecord> &records,
                                QString &error) override;
    bool readFilesUnderFolder(const QString &folderPath, QList<LocalFileRecognitionRecord> &records,
                              QString &error) override;
    bool readRecognitionCatalog(QList<Media> &media, QString &error) override;
    bool saveRecognitionBatch(const QList<LocalFileRecognitionRecord> &records, QString &error) override;
private:
    QSqlDatabase database_;
};

#endif
