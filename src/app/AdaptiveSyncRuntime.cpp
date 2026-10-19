#include "AdaptiveSyncRuntime.h"

#include <QMetaObject>
#include <QMutexLocker>

#include <utility>

namespace {
QString SafeError(const QString &error, const QString &fallback) {
    return error.isEmpty() ? fallback : error;
}
}

AdaptiveSyncRuntime::AdaptiveSyncRuntime(RepositoryFactory repositoryFactory, ExecutorFactory executorFactory,
                                         std::map<SyncTaskKind, SyncSchedulePolicy> policies) {
    qRegisterMetaType<SyncProgress>();
    owner_ = new QObject;
    owner_->moveToThread(&thread_);
    thread_.start(QThread::LowPriority);
    QMetaObject::invokeMethod(owner_, [this, repositoryFactory = std::move(repositoryFactory),
                                       executorFactory = std::move(executorFactory), policies = std::move(policies)]() mutable {
        Initialize(std::move(repositoryFactory), std::move(executorFactory), std::move(policies));
    }, Qt::QueuedConnection);
}

AdaptiveSyncRuntime::~AdaptiveSyncRuntime() {
    shutdown();
    if (thread_.isRunning()) thread_.wait();
}

void AdaptiveSyncRuntime::shutdown() {
    QObject *owner = nullptr;
    {
        QMutexLocker lock(&stateMutex_);
        if (state_ == State::Stopped || state_ == State::Failed || stopRequested_) return;
        stopRequested_ = true;
        state_ = State::Stopping;
        owner = owner_;
    }
    if (owner) QMetaObject::invokeMethod(owner, [this] { StopOnWorker(); }, Qt::QueuedConnection);
}

bool AdaptiveSyncRuntime::isReady() const {
    QMutexLocker lock(&stateMutex_);
    return state_ == State::Ready;
}

void AdaptiveSyncRuntime::start() {
    QObject *owner = nullptr;
    {
        QMutexLocker lock(&stateMutex_);
        if (state_ == State::Stopped || state_ == State::Failed || stopRequested_ || startRequested_) return;
        startRequested_ = true;
        owner = owner_;
    }
    if (isReady() && owner) QMetaObject::invokeMethod(owner, [this] { StartOnWorker(); }, Qt::QueuedConnection);
}

void AdaptiveSyncRuntime::requestNow(const SyncPartition partition) {
    QObject *owner = nullptr;
    bool ready = false;
    {
        QMutexLocker lock(&stateMutex_);
        if (state_ == State::Stopped || state_ == State::Failed || stopRequested_) return;
        startRequested_ = true;
        ready = state_ == State::Ready;
        if (!ready) pendingRequests_.insert(partition);
        owner = owner_;
    }
    if (ready && owner) {
        QMetaObject::invokeMethod(owner, [this, partition] { RequestNowOnWorker(partition); }, Qt::QueuedConnection);
    }
}

void AdaptiveSyncRuntime::setNormalInterval(const SyncTaskKind kind, const std::chrono::milliseconds interval) {
    QObject *owner = nullptr;
    {
        QMutexLocker lock(&stateMutex_);
        if (state_ == State::Stopped || state_ == State::Failed || stopRequested_) return;
        owner = owner_;
    }
    // Queued after Initialize on the same object, so the coordinator already exists when this runs.
    if (owner) {
        QMetaObject::invokeMethod(owner, [this, kind, interval] {
            if (coordinator_) coordinator_->SetNormalInterval(kind, interval);
        }, Qt::QueuedConnection);
    }
}

void AdaptiveSyncRuntime::cancel(const SyncPartition partition, std::function<void()> acknowledged) {
    QObject *owner = nullptr;
    {
        QMutexLocker lock(&stateMutex_);
        if (state_ == State::Ready && !stopRequested_) owner = owner_;
    }
    if (!owner) {
        if (acknowledged) acknowledged();
        return;
    }
    // The acknowledgement is queued onto the runtime object (created on the caller's/GUI thread), not onto
    // the worker, so the caller's continuation always runs on its own thread.
    QMetaObject::invokeMethod(owner, [this, partition, acknowledged = std::move(acknowledged)]() mutable {
        CancelOnWorker(partition, std::move(acknowledged));
    }, Qt::QueuedConnection);
}

bool AdaptiveSyncRuntime::isStopped() const {
    QMutexLocker lock(&stateMutex_);
    return (state_ == State::Stopped || state_ == State::Failed) && !thread_.isRunning();
}

QString AdaptiveSyncRuntime::initializationError() const {
    QMutexLocker lock(&stateMutex_);
    return initializationError_;
}

template <typename... Args>
void AdaptiveSyncRuntime::Relay(void (AdaptiveSyncCoordinator::*signal)(Args...),
                                void (AdaptiveSyncRuntime::*relayed)(Args...)) {
    // The coordinator emits on the worker thread; re-emit on this object's thread with copied arguments.
    connect(coordinator_.get(), signal, owner_, [this, relayed](Args... arguments) {
        QMetaObject::invokeMethod(this, [this, relayed, arguments...] {
            emit (this->*relayed)(arguments...);
        }, Qt::QueuedConnection);
    });
}

void AdaptiveSyncRuntime::Initialize(RepositoryFactory repositoryFactory, ExecutorFactory executorFactory,
                                     std::map<SyncTaskKind, SyncSchedulePolicy> policies) {
    QString error;
    repository_ = repositoryFactory(error);
    if (!repository_) {
        DisposeOnWorker(SafeError(error, QStringLiteral("Unable to initialize synchronization task state.")));
        return;
    }
    executor_ = executorFactory();
    if (!executor_) {
        DisposeOnWorker(QStringLiteral("Unable to initialize synchronization task execution."));
        return;
    }
    if (isStopRequested()) {
        DisposeOnWorker();
        return;
    }

    coordinator_ = std::make_unique<AdaptiveSyncCoordinator>(*repository_, *executor_,
                                                               AdaptiveSyncCoordinator::Clock{}, std::move(policies), true);
    connect(coordinator_.get(), &AdaptiveSyncCoordinator::Stopped, owner_, [this] { DisposeOnWorker(); });
    Relay(&AdaptiveSyncCoordinator::TaskStarted, &AdaptiveSyncRuntime::BackgroundTaskStarted);
    Relay(&AdaptiveSyncCoordinator::TaskProgress, &AdaptiveSyncRuntime::BackgroundTaskProgress);
    Relay(&AdaptiveSyncCoordinator::TaskCompleted, &AdaptiveSyncRuntime::BackgroundTaskCompleted);
    Relay(&AdaptiveSyncCoordinator::TaskCancelled, &AdaptiveSyncRuntime::BackgroundTaskCancelled);
    Relay(&AdaptiveSyncCoordinator::TaskFailed, &AdaptiveSyncRuntime::BackgroundTaskFailed);

    if (isStopRequested()) {
        StopOnWorker();
        return;
    }
    bool startRequested = false;
    std::set<SyncPartition> pendingRequests;
    {
        QMutexLocker lock(&stateMutex_);
        if (state_ == State::Initializing) state_ = State::Ready;
        startRequested = startRequested_;
        pendingRequests = std::move(pendingRequests_);
        pendingRequests_.clear();
    }
    QMetaObject::invokeMethod(this, [this] { emit Ready(); }, Qt::QueuedConnection);
    if (startRequested) StartOnWorker();
    for (const auto partition : pendingRequests) RequestNowOnWorker(partition);
}

void AdaptiveSyncRuntime::StartOnWorker() {
    if (isStopRequested() || !coordinator_ || coordinatorStarted_) return;
    QString schedulingError;
    const auto schedulingFailure = connect(coordinator_.get(), &AdaptiveSyncCoordinator::SchedulingFailed,
                                           owner_, [&schedulingError](const QString &error) {
                                               schedulingError = error;
                                           });
    const bool started = coordinator_->Start();
    disconnect(schedulingFailure);
    if (started) {
        coordinatorStarted_ = true;
    } else if (!isStopRequested()) {
        DisposeOnWorker(SafeError(schedulingError, QStringLiteral("Unable to load synchronization task state.")));
    }
}

void AdaptiveSyncRuntime::RequestNowOnWorker(const SyncPartition partition) {
    StartOnWorker();
    if (!isStopRequested() && coordinator_ && coordinatorStarted_) coordinator_->RequestNow(partition);
}

void AdaptiveSyncRuntime::CancelOnWorker(const SyncPartition partition, std::function<void()> acknowledged) {
    auto deliver = [this, acknowledged = std::move(acknowledged)]() mutable {
        QMetaObject::invokeMethod(this, [acknowledged = std::move(acknowledged)] {
            if (acknowledged) acknowledged();
        }, Qt::QueuedConnection);
    };
    if (isStopRequested() || !coordinator_ || !coordinatorStarted_) {
        deliver();
        return;
    }
    coordinator_->Cancel(partition, std::move(deliver));
}

void AdaptiveSyncRuntime::StopOnWorker() {
    if (coordinator_) {
        coordinator_->Stop();
        return;
    }
    DisposeOnWorker();
}

void AdaptiveSyncRuntime::DisposeOnWorker(QString initializationFailure) {
    if (disposalScheduled_) return;
    disposalScheduled_ = true;
    QMetaObject::invokeMethod(owner_, [this, initializationFailure = std::move(initializationFailure)] {
        coordinator_.reset();
        executor_.reset();
        repository_.reset();
        {
            QMutexLocker lock(&stateMutex_);
            if (initializationFailure.isEmpty()) {
                state_ = State::Stopped;
            } else {
                state_ = State::Failed;
                initializationError_ = initializationFailure;
            }
        }
        QMetaObject::invokeMethod(this, [this, initializationFailure] {
            if (initializationFailure.isEmpty()) emit Stopped();
            else emit InitializationFailed(initializationFailure);
        }, Qt::QueuedConnection);
        owner_->deleteLater();
        owner_ = nullptr;
        thread_.quit();
    }, Qt::QueuedConnection);
}

bool AdaptiveSyncRuntime::isStopRequested() const {
    QMutexLocker lock(&stateMutex_);
    return stopRequested_;
}
