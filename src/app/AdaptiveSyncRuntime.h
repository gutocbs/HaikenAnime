#ifndef HAIKENANIME_ADAPTIVESYNCRUNTIME_H
#define HAIKENANIME_ADAPTIVESYNCRUNTIME_H

#include <QObject>
#include <QMutex>
#include <QThread>

#include <functional>
#include <memory>

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

    void shutdown();
    [[nodiscard]] bool isReady() const;
    [[nodiscard]] bool isStopped() const;
    [[nodiscard]] QString initializationError() const;

signals:
    void Ready();
    void InitializationFailed(const QString &safeError);
    void Stopped();

private:
    enum class State { Initializing, Ready, Stopping, Stopped, Failed };

    void Initialize(RepositoryFactory repositoryFactory, ExecutorFactory executorFactory,
                    std::map<SyncTaskKind, SyncSchedulePolicy> policies);
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
    bool disposalScheduled_ = false;
};

#endif // HAIKENANIME_ADAPTIVESYNCRUNTIME_H
