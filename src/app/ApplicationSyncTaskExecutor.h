#ifndef HAIKENANIME_APPLICATIONSYNCTASKEXECUTOR_H
#define HAIKENANIME_APPLICATIONSYNCTASKEXECUTOR_H

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <QUrl>

#include "../application/scheduling/ISyncTaskExecutor.h"
#include "../application/anilist/AniListCredentials.h"

class QThread;
class SqliteDatabase;

/** Creates each synchronization task's network and SQLite dependencies on its worker thread. */
class ApplicationSyncTaskExecutor final : public ISyncTaskExecutor {
public:
    struct AuthenticatedUserListConfiguration final {
        QUrl endpoint;
        std::function<AniListCredentials()> credentialsProvider;
        int perChunk = 100;
        int httpTimeoutMs = 30000;
        int httpMaxRetries = 0;
        int httpRetryDelayMs = 0;
        std::function<void(const QString &)> auditLogger;
    };

    ApplicationSyncTaskExecutor(QString databasePath, int timeoutMs,
                                AuthenticatedUserListConfiguration authenticatedUserList);
    ~ApplicationSyncTaskExecutor() override;

    void SetProgressReporter(ProgressReporter reporter) override;
    void Execute(const SyncTaskState &state, qint64 generation, Completion completion) override;
    void Cancel(SyncPartition partition, CancellationAcknowledgement acknowledgement) override;
    void Shutdown(ShutdownAcknowledgement acknowledgement) override;

private:
    struct Job final {
        std::atomic_bool cancelled = false;
        QThread *thread = nullptr;
        CancellationAcknowledgement cancellationAcknowledgement;
    };

    using StagePublisher = std::function<void(const QString &stageKey, int stageIndex, int stageCount,
                                              int processedItems, int totalItems)>;

    /** Downloads the enabled anime and manga lists, checkpointing each page, then reconciles the snapshot. */
    void RunUserListTask(SqliteDatabase &database, const AniListCredentials &credentials,
                         const std::shared_ptr<Job> &job, SyncTaskState &checkpointState,
                         const StagePublisher &publish, SyncTaskExecutionResult &result, QString &error) const;
    /** Drains the account's outbox to AniList and reports progress outcomes to the recognition history. */
    void RunPendingChangesTask(SqliteDatabase &database, const AniListCredentials &credentials,
                               const std::shared_ptr<Job> &job, const StagePublisher &publish,
                               SyncTaskExecutionResult &result, QString &error) const;
    void Audit(const QString &message) const;
    void Finish(const std::shared_ptr<Job> &job, SyncPartition partition,
                const Completion &completion, SyncTaskExecutionResult result);
    void NotifyShutdownIfIdle();

    QString databasePath_;
    int timeoutMs_ = 0;
    AuthenticatedUserListConfiguration authenticatedUserList_;
    std::mutex mutex_;
    std::map<SyncPartition, std::shared_ptr<Job>> jobs_;
    ShutdownAcknowledgement shutdownAcknowledgement_;
    ProgressReporter progressReporter_;
};

#endif // HAIKENANIME_APPLICATIONSYNCTASKEXECUTOR_H
