#ifndef HAIKENANIME_APPLICATIONSYNCTASKEXECUTOR_H
#define HAIKENANIME_APPLICATIONSYNCTASKEXECUTOR_H

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "../application/scheduling/ISyncTaskExecutor.h"

class QThread;

/** Creates each synchronization task's network and SQLite dependencies on its worker thread. */
class ApplicationSyncTaskExecutor final : public ISyncTaskExecutor {
public:
    ApplicationSyncTaskExecutor(QString databasePath, QString userListFixturePath, QString catalogFixturePath,
                                QString upsertQueryPath,
                                QString readQueryPath, QString readActiveMediaIdsQueryPath,
                                QString markSourceRemovedQueryPath, QString readTaskStatesQueryPath,
                                QString upsertTaskStateQueryPath, QString deleteTaskStateQueryPath,
                                int timeoutMs);
    ~ApplicationSyncTaskExecutor() override;

    void Execute(const SyncTaskState &state, qint64 generation, Completion completion) override;
    void Cancel(SyncPartition partition, CancellationAcknowledgement acknowledgement) override;
    void Shutdown(ShutdownAcknowledgement acknowledgement) override;

private:
    struct Job final {
        std::atomic_bool cancelled = false;
        QThread *thread = nullptr;
        CancellationAcknowledgement cancellationAcknowledgement;
    };

    void Finish(const std::shared_ptr<Job> &job, SyncPartition partition,
                const Completion &completion, SyncTaskExecutionResult result);
    void NotifyShutdownIfIdle();

    QString databasePath_;
    QString userListFixturePath_;
    QString catalogFixturePath_;
    QString upsertQueryPath_;
    QString readQueryPath_;
    QString readActiveMediaIdsQueryPath_;
    QString markSourceRemovedQueryPath_;
    QString readTaskStatesQueryPath_;
    QString upsertTaskStateQueryPath_;
    QString deleteTaskStateQueryPath_;
    int timeoutMs_ = 0;
    std::mutex mutex_;
    std::map<SyncPartition, std::shared_ptr<Job>> jobs_;
    ShutdownAcknowledgement shutdownAcknowledgement_;
};

#endif // HAIKENANIME_APPLICATIONSYNCTASKEXECUTOR_H
