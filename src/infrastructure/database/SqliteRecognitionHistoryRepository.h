#ifndef HAIKENANIME_SQLITERECOGNITIONHISTORYREPOSITORY_H
#define HAIKENANIME_SQLITERECOGNITIONHISTORYREPOSITORY_H

#include <QSqlDatabase>

#include "../../application/recognition/IRecognitionHistoryRepository.h"

class SqliteRecognitionHistoryRepository final : public IRecognitionHistoryRepository {
public:
    SqliteRecognitionHistoryRepository(QSqlDatabase database, QString readQuery, QString insertQuery,
                                       QString updateQuery);

    bool insert(RecognitionRecord &record, QString &error) override;
    bool update(const RecognitionRecord &record, QString &error) override;
    bool find(const RecognitionHistoryQuery &query, QList<RecognitionRecord> &records, QString &error) override;

private:
    QSqlDatabase database_;
    QString readQuery_;
    QString insertQuery_;
    QString updateQuery_;
};

#endif // HAIKENANIME_SQLITERECOGNITIONHISTORYREPOSITORY_H
