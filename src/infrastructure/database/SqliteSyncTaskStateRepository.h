#ifndef HAIKENANIME_SQLITESYNCTASKSTATEREPOSITORY_H
#define HAIKENANIME_SQLITESYNCTASKSTATEREPOSITORY_H

#include <QSqlDatabase>

#include "../../application/scheduling/ISyncTaskStateRepository.h"

// Not final: the composition derives from it to own the worker connection.
class SqliteSyncTaskStateRepository : public ISyncTaskStateRepository {
public:
    explicit SqliteSyncTaskStateRepository(QSqlDatabase database);

    bool ReadAll(QList<SyncTaskState> &states, QString &error) override;
    bool Upsert(const SyncTaskState &state, QString &error) override;

private:
    QSqlDatabase database_;
};

#endif // HAIKENANIME_SQLITESYNCTASKSTATEREPOSITORY_H
