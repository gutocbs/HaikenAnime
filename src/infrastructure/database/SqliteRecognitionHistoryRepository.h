#ifndef HAIKENANIME_SQLITERECOGNITIONHISTORYREPOSITORY_H
#define HAIKENANIME_SQLITERECOGNITIONHISTORYREPOSITORY_H

#include <QSqlDatabase>

#include "../../application/recognition/IRecognitionHistoryRepository.h"

class SqliteRecognitionHistoryRepository final : public IRecognitionHistoryRepository {
public:
    SqliteRecognitionHistoryRepository(
        QSqlDatabase database, QString readQuery, QString insertQuery, QString updateQuery,
        QString deleteQuery = QStringLiteral(":/sqlite/queries/delete-recognition-record.sql"),
        QString discardProgressQuery = QStringLiteral(":/sqlite/queries/discard-recognition-progress.sql"),
        QString queuedProgressQuery = QStringLiteral(":/sqlite/queries/find-queued-recognition-progress.sql"));

    bool insert(RecognitionRecord &record, QString &error) override;
    bool update(const RecognitionRecord &record, QString &error) override;
    bool find(const RecognitionHistoryQuery &query, QList<RecognitionRecord> &records, QString &error) override;
    bool remove(qint64 recordId, const std::optional<QueuedProgress> &queuedProgress, QString &error) override;
    bool hasQueuedProgress(const QueuedProgress &progress, bool &found, QString &error) override;

private:
    QSqlDatabase database_;
    QString readQuery_;
    QString insertQuery_;
    QString updateQuery_;
    QString deleteQuery_;
    QString discardProgressQuery_;
    QString queuedProgressQuery_;
};

#endif // HAIKENANIME_SQLITERECOGNITIONHISTORYREPOSITORY_H
