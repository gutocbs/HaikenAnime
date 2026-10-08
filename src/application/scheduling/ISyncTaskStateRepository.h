#ifndef HAIKENANIME_ISYNCTASKSTATEREPOSITORY_H
#define HAIKENANIME_ISYNCTASKSTATEREPOSITORY_H

#include <QList>
#include <QString>

#include "SyncTaskTypes.h"

class ISyncTaskStateRepository {
public:
    virtual ~ISyncTaskStateRepository() = default;
    virtual bool ReadAll(QList<SyncTaskState> &states, QString &error) = 0;
    virtual bool Upsert(const SyncTaskState &state, QString &error) = 0;
    virtual bool Remove(const SyncPartition &partition, QString &error) = 0;
};

#endif // HAIKENANIME_ISYNCTASKSTATEREPOSITORY_H
