#ifndef HAIKENANIME_SQLITERECOGNITIONHISTORYREPOSITORY_H
#define HAIKENANIME_SQLITERECOGNITIONHISTORYREPOSITORY_H

#include <QSqlDatabase>

#include "../../application/recognition/IRecognitionHistoryRepository.h"

class SqliteRecognitionHistoryRepository final : public IRecognitionHistoryRepository {
public:
    explicit SqliteRecognitionHistoryRepository(QSqlDatabase database);

    bool insert(RecognitionRecord &record, QString &error) override;
    bool update(const RecognitionRecord &record, QString &error) override;
    bool find(const RecognitionHistoryQuery &query, QList<RecognitionRecord> &records, QString &error) override;
    bool remove(qint64 recordId, const std::optional<QueuedProgress> &queuedProgress, QString &error) override;
    bool hasQueuedProgress(const QueuedProgress &progress, bool &found, QString &error) override;

private:
    QSqlDatabase database_;
};

#endif // HAIKENANIME_SQLITERECOGNITIONHISTORYREPOSITORY_H
