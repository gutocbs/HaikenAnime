#include "AdaptiveSyncRuntime.h"

#include <QMetaObject>

AdaptiveSyncRuntime::AdaptiveSyncRuntime(RepositoryFactory repositoryFactory, ExecutorFactory executorFactory,
                                         std::map<SyncTaskKind, SyncSchedulePolicy> policies) {
    owner_ = new QObject;
    owner_->moveToThread(&thread_);
    thread_.start(QThread::LowPriority);
    QMetaObject::invokeMethod(owner_, [this, repositoryFactory = std::move(repositoryFactory),
                                       executorFactory = std::move(executorFactory), policies = std::move(policies)]() mutable {
        QString error;
        repository_ = repositoryFactory(error);
        executor_ = executorFactory();
        if (!repository_ || !executor_) return;
        coordinator_ = std::make_unique<AdaptiveSyncCoordinator>(*repository_, *executor_,
                                                                   AdaptiveSyncCoordinator::Clock{},
                                                                   std::move(policies));
        coordinator_->Start();
    }, Qt::BlockingQueuedConnection);
}

AdaptiveSyncRuntime::~AdaptiveSyncRuntime() {
    shutdown();
}

void AdaptiveSyncRuntime::shutdown() {
    if (!owner_) return;
    QMetaObject::invokeMethod(owner_, [this] {
        if (coordinator_) coordinator_->Stop();
        coordinator_.reset();
        executor_.reset();
        repository_.reset();
    }, Qt::BlockingQueuedConnection);
    QMetaObject::invokeMethod(owner_, [this] { delete owner_; owner_ = nullptr; }, Qt::BlockingQueuedConnection);
    thread_.quit();
    thread_.wait();
}
