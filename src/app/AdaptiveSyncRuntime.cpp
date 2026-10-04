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

bool AdaptiveSyncRuntime::isStopped() const {
    QMutexLocker lock(&stateMutex_);
    return (state_ == State::Stopped || state_ == State::Failed) && !thread_.isRunning();
}

QString AdaptiveSyncRuntime::initializationError() const {
    QMutexLocker lock(&stateMutex_);
    return initializationError_;
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
                                                               AdaptiveSyncCoordinator::Clock{}, std::move(policies));
    QString schedulingError;
    const auto schedulingFailure = connect(coordinator_.get(), &AdaptiveSyncCoordinator::SchedulingFailed,
                                           owner_, [&schedulingError](const QString &error) {
                                               schedulingError = error;
                                           });
    const bool started = coordinator_->Start();
    disconnect(schedulingFailure);
    if (!started) {
        DisposeOnWorker(SafeError(schedulingError, QStringLiteral("Unable to load synchronization task state.")));
        return;
    }
    connect(coordinator_.get(), &AdaptiveSyncCoordinator::Stopped, owner_, [this] { DisposeOnWorker(); });

    if (isStopRequested()) {
        StopOnWorker();
        return;
    }
    {
        QMutexLocker lock(&stateMutex_);
        if (state_ == State::Initializing) state_ = State::Ready;
    }
    QMetaObject::invokeMethod(this, [this] { emit Ready(); }, Qt::QueuedConnection);
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
