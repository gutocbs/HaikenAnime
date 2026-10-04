#ifndef HAIKENANIME_ADAPTIVESYNCRUNTIME_H
#define HAIKENANIME_ADAPTIVESYNCRUNTIME_H

#include <QObject>
#include <QThread>

#include <functional>
#include <memory>

#include "AdaptiveSyncCoordinator.h"

/** Owns the coordinator, its task-state repository, and the scheduler event loop off the UI thread. */
class AdaptiveSyncRuntime final {
public:
    using RepositoryFactory = std::function<std::unique_ptr<ISyncTaskStateRepository>(QString &error)>;
    using ExecutorFactory = std::function<std::unique_ptr<ISyncTaskExecutor>()>;

    AdaptiveSyncRuntime(RepositoryFactory repositoryFactory, ExecutorFactory executorFactory,
                        std::map<SyncTaskKind, SyncSchedulePolicy> policies);
    ~AdaptiveSyncRuntime();

    AdaptiveSyncRuntime(const AdaptiveSyncRuntime &) = delete;
    AdaptiveSyncRuntime &operator=(const AdaptiveSyncRuntime &) = delete;

    void shutdown();

private:
    QThread thread_;
    QObject *owner_ = nullptr;
    std::unique_ptr<ISyncTaskStateRepository> repository_;
    std::unique_ptr<ISyncTaskExecutor> executor_;
    std::unique_ptr<AdaptiveSyncCoordinator> coordinator_;
};

#endif // HAIKENANIME_ADAPTIVESYNCRUNTIME_H
