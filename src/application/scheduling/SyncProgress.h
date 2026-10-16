#ifndef HAIKENANIME_SYNCPROGRESS_H
#define HAIKENANIME_SYNCPROGRESS_H

#include <QMetaType>
#include <QString>

#include "SyncTaskTypes.h"

/** Snapshot of how far one synchronization task has advanced; totalItems is -1 when unknown. */
struct SyncProgress final {
    SyncPartition partition = SyncPartition::UserList;
    QString stageKey;
    int stageIndex = 0;
    int stageCount = 0;
    int processedItems = 0;
    int totalItems = -1;
};

Q_DECLARE_METATYPE(SyncProgress)

#endif // HAIKENANIME_SYNCPROGRESS_H
