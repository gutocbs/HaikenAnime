#ifndef HAIKENANIME_ISYNCTASKEXECUTOR_H
#define HAIKENANIME_ISYNCTASKEXECUTOR_H

#include <QString>

#include <functional>
#include <optional>

#include "SyncTaskTypes.h"

struct SyncTaskExecutionResult final {
    bool succeeded = false;
    AniListSyncErrorCategory errorCategory = AniListSyncErrorCategory::None;
    QString safeErrorDetail;
    std::optional<int> confirmedPage;
    std::optional<QString> confirmedCursor;
};

/** Starts one task on the executor-owned worker thread and completes asynchronously. */
class ISyncTaskExecutor {
public:
    using Completion = std::function<void(const SyncTaskExecutionResult &)>;

    virtual ~ISyncTaskExecutor() = default;
    virtual void Execute(const SyncTaskState &state, qint64 generation, Completion completion) = 0;
    virtual void Cancel(SyncPartition partition) = 0;
};

#endif // HAIKENANIME_ISYNCTASKEXECUTOR_H
