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
    std::optional<QDateTime> retryAfter;
    std::optional<int> confirmedPage;
    std::optional<QString> confirmedCursor;
};

/** Starts one task on the executor-owned worker thread and completes asynchronously. */
class ISyncTaskExecutor {
public:
    using Completion = std::function<void(const SyncTaskExecutionResult &)>;
    using CancellationAcknowledgement = std::function<void()>;
    using ShutdownAcknowledgement = std::function<void()>;

    virtual ~ISyncTaskExecutor() = default;
    virtual void Execute(const SyncTaskState &state, qint64 generation, Completion completion) = 0;
    // Acknowledgement means the worker has accounted for the in-flight task
    // and will not later invoke its completion callback for this generation.
    virtual void Cancel(SyncPartition partition, CancellationAcknowledgement acknowledgement) = 0;
    // Acknowledgement means all executor-owned workers have stopped.
    virtual void Shutdown(ShutdownAcknowledgement acknowledgement) = 0;
};

#endif // HAIKENANIME_ISYNCTASKEXECUTOR_H
