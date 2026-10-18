#ifndef HAIKENANIME_ADAPTIVESYNCRUNTIME_H
#define HAIKENANIME_ADAPTIVESYNCRUNTIME_H

#include <QObject>
#include <QMutex>
#include <QThread>

#include <functional>
#include <memory>
#include <set>

#include "AdaptiveSyncCoordinator.h"

/** Owns the coordinator, its task-state repository, and the scheduler event loop off the UI thread. */
class AdaptiveSyncRuntime final : public QObject {
    Q_OBJECT
public:
    using RepositoryFactory = std::function<std::unique_ptr<ISyncTaskStateRepository>(QString &error)>;
    using ExecutorFactory = std::function<std::unique_ptr<ISyncTaskExecutor>()>;

    AdaptiveSyncRuntime(RepositoryFactory repositoryFactory, ExecutorFactory executorFactory,
                        std::map<SyncTaskKind, SyncSchedulePolicy> policies);
    ~AdaptiveSyncRuntime();

    AdaptiveSyncRuntime(const AdaptiveSyncRuntime &) = delete;
    AdaptiveSyncRuntime &operator=(const AdaptiveSyncRuntime &) = delete;

    void start();
    void requestNow(SyncPartition partition);
    /**
     * Cancels the partition's active task on the worker thread. `acknowledged` is delivered through a
     * queued call on this runtime object, which lives on the thread that created it (the GUI thread), so
     * the caller's continuation never runs on the worker. When the runtime is not ready or is stopping
     * there is nothing to cancel and `acknowledged` runs immediately on the calling thread.
     */
    void cancel(SyncPartition partition, std::function<void()> acknowledged);
    void shutdown();
    [[nodiscard]] bool isReady() const;
    [[nodiscard]] bool isStopped() const;
    [[nodiscard]] QString initializationError() const;

signals:
    void Ready();
    void InitializationFailed(const QString &safeError);
    void BackgroundTaskStarted(SyncPartition partition);
    void BackgroundTaskProgress(SyncProgress progress);
    void BackgroundTaskCompleted(SyncPartition partition);
    void BackgroundTaskCancelled(SyncPartition partition);
    void BackgroundTaskFailed(SyncPartition partition, const QString &safeError);
    void Stopped();

private:
    enum class State { Initializing, Ready, Stopping, Stopped, Failed };

    void Initialize(RepositoryFactory repositoryFactory, ExecutorFactory executorFactory,
                    std::map<SyncTaskKind, SyncSchedulePolicy> policies);
    /** Forwards one coordinator signal to the GUI-facing signal with the same arguments. */
    template <typename... Args>
    void Relay(void (AdaptiveSyncCoordinator::*signal)(Args...), void (AdaptiveSyncRuntime::*relayed)(Args...));
    void StartOnWorker();
    void RequestNowOnWorker(SyncPartition partition);
    void CancelOnWorker(SyncPartition partition, std::function<void()> acknowledged);
    void StopOnWorker();
    void DisposeOnWorker(QString initializationFailure = {});
    [[nodiscard]] bool isStopRequested() const;

    QThread thread_;
    QObject *owner_ = nullptr;
    std::unique_ptr<ISyncTaskStateRepository> repository_;
    std::unique_ptr<ISyncTaskExecutor> executor_;
    std::unique_ptr<AdaptiveSyncCoordinator> coordinator_;
    mutable QMutex stateMutex_;
    State state_ = State::Initializing;
    QString initializationError_;
    bool stopRequested_ = false;
    bool startRequested_ = false;
    bool coordinatorStarted_ = false;
    std::set<SyncPartition> pendingRequests_;
    bool disposalScheduled_ = false;
};

#endif // HAIKENANIME_ADAPTIVESYNCRUNTIME_H
