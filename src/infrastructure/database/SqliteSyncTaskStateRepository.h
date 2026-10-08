#ifndef HAIKENANIME_SQLITESYNCTASKSTATEREPOSITORY_H
#define HAIKENANIME_SQLITESYNCTASKSTATEREPOSITORY_H

#include <QSqlDatabase>

#include "../../application/scheduling/ISyncTaskStateRepository.h"

class SqliteSyncTaskStateRepository final : public ISyncTaskStateRepository {
public:
    SqliteSyncTaskStateRepository(QSqlDatabase database, QString readQuery, QString upsertQuery, QString deleteQuery);

    bool ReadAll(QList<SyncTaskState> &states, QString &error) override;
    bool Upsert(const SyncTaskState &state, QString &error) override;
    bool Remove(const SyncPartition &partition, QString &error) override;

private:
    QSqlDatabase database_;
    QString readQuery_;
    QString upsertQuery_;
    QString deleteQuery_;
};

#endif // HAIKENANIME_SQLITESYNCTASKSTATEREPOSITORY_H
