#include <QElapsedTimer>
#include <QSemaphore>
#include <QSignalSpy>
#include <QtTest>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

#include "../../src/app/AdaptiveSyncRuntime.h"

namespace {
class FakeTaskStateRepository final : public ISyncTaskStateRepository {
public:
    explicit FakeTaskStateRepository(QList<SyncTaskState> initialStates = {})
        : states(std::move(initialStates)) {}

    bool ReadAll(QList<SyncTaskState> &states, QString &error) override {
        states = this->states;
        error.clear();
        return true;
    }
    bool Upsert(const SyncTaskState &, QString &error) override {
        error.clear();
        return true;
    }
    bool Remove(const SyncPartition &, QString &error) override {
        error.clear();
        return true;
    }

    QList<SyncTaskState> states;
};

struct ExecutorState final {
    std::atomic_bool shutdownRequested = false;
    std::atomic_int executionCount = 0;
    std::atomic_int cancellationCount = 0;
    std::mutex completionMutex;
    ISyncTaskExecutor::Completion completion;
    ISyncTaskExecutor::ShutdownAcknowledgement shutdownAcknowledgement;
};

class FakeTaskExecutor final : public ISyncTaskExecutor {
public:
    explicit FakeTaskExecutor(std::shared_ptr<ExecutorState> state) : state_(std::move(state)) {}

    void Execute(const SyncTaskState &, qint64, Completion completion) override {
        {
            std::lock_guard lock(state_->completionMutex);
            state_->completion = std::move(completion);
        }
        ++state_->executionCount;
    }
    void Cancel(SyncPartition, CancellationAcknowledgement acknowledgement) override {
        ++state_->cancellationCount;
        acknowledgement();
    }
    void Shutdown(ShutdownAcknowledgement acknowledgement) override {
        state_->shutdownAcknowledgement = std::move(acknowledgement);
        state_->shutdownRequested = true;
    }

private:
    std::shared_ptr<ExecutorState> state_;
};

std::unique_ptr<ISyncTaskStateRepository> readyRepository(QString &error) {
    error.clear();
    return std::make_unique<FakeTaskStateRepository>();
}

std::unique_ptr<ISyncTaskStateRepository> dueRepository(QString &error) {
    error.clear();
    SyncTaskState state;
    state.kind = SyncTaskKind::UserList;
    state.partition = SyncPartition::UserList;
    state.cacheValidity = CacheValidity::Expired;
    return std::make_unique<FakeTaskStateRepository>(QList<SyncTaskState>{state});
}

std::unique_ptr<ISyncTaskStateRepository> freshUserListRepository(QString &error) {
    error.clear();
    SyncTaskState state;
    state.kind = SyncTaskKind::UserList;
    state.partition = SyncPartition::UserList;
    state.status = SyncTaskStatus::Succeeded;
    state.cacheValidity = CacheValidity::Fresh;
    state.lastSucceededAt = QDateTime::currentDateTimeUtc();
    state.nextRunAt = state.lastSucceededAt->addDays(1);
    return std::make_unique<FakeTaskStateRepository>(QList<SyncTaskState>{state});
}
}

class AdaptiveSyncRuntimeTests final : public QObject {
    Q_OBJECT
private slots:
    void constructorQueuesInitializationWithoutBlockingCaller();
    void startsQueuedBeforeInitializationOnlyOnce();
    void startsPersistedWorkOnlyAfterExplicitActivation();
    void forcesFreshUserListSynchronizationWhenRequested();
    void drainsAnActiveExecutorBeforeStopping();
    void exposesRepositoryInitializationFailure();
    void propagatesCompletedBackgroundTask();
    void defersWorkerDestructionUntilCoordinatorStops();
    void destructorRetainsWorkerOwnershipPastLegacyTimeout();
};

void AdaptiveSyncRuntimeTests::constructorQueuesInitializationWithoutBlockingCaller() {
    QSemaphore initializationGate;
    const auto executorState = std::make_shared<ExecutorState>();
    QElapsedTimer elapsed;
    elapsed.start();
    AdaptiveSyncRuntime runtime(
        [&initializationGate](QString &error) {
            initializationGate.acquire();
            return readyRepository(error);
        },
        [executorState] { return std::make_unique<FakeTaskExecutor>(executorState); }, {});

    QVERIFY(elapsed.elapsed() < 50);
    QVERIFY(!runtime.isReady());

    QSignalSpy ready(&runtime, &AdaptiveSyncRuntime::Ready);
    initializationGate.release();
    QTRY_COMPARE(ready.count(), 1);
    QVERIFY(runtime.isReady());

    runtime.shutdown();
    QTRY_VERIFY(executorState->shutdownRequested.load());
    executorState->shutdownAcknowledgement();
    QTRY_VERIFY(runtime.isStopped());
}

void AdaptiveSyncRuntimeTests::startsQueuedBeforeInitializationOnlyOnce() {
    QSemaphore initializationGate;
    const auto executorState = std::make_shared<ExecutorState>();
    AdaptiveSyncRuntime runtime(
        [&initializationGate](QString &error) {
            initializationGate.acquire();
            return dueRepository(error);
        },
        [executorState] { return std::make_unique<FakeTaskExecutor>(executorState); }, {});

    runtime.start();
    initializationGate.release();
    QTRY_VERIFY(runtime.isReady());
    QTRY_COMPARE(executorState->executionCount.load(), 1);
    runtime.start();
    QTest::qWait(20);
    QCOMPARE(executorState->executionCount.load(), 1);

    runtime.shutdown();
    QTRY_VERIFY(executorState->shutdownRequested.load());
    executorState->shutdownAcknowledgement();
    QTRY_VERIFY(runtime.isStopped());
}

void AdaptiveSyncRuntimeTests::startsPersistedWorkOnlyAfterExplicitActivation() {
    const auto executorState = std::make_shared<ExecutorState>();
    AdaptiveSyncRuntime runtime(
        dueRepository,
        [executorState] { return std::make_unique<FakeTaskExecutor>(executorState); }, {});

    QTRY_VERIFY(runtime.isReady());
    QCOMPARE(executorState->executionCount.load(), 0);

    runtime.start();
    QTRY_COMPARE(executorState->executionCount.load(), 1);

    runtime.shutdown();
    QTRY_VERIFY(executorState->shutdownRequested.load());
    executorState->shutdownAcknowledgement();
    QTRY_VERIFY(runtime.isStopped());
}

void AdaptiveSyncRuntimeTests::forcesFreshUserListSynchronizationWhenRequested() {
    const auto executorState = std::make_shared<ExecutorState>();
    AdaptiveSyncRuntime runtime(
        freshUserListRepository,
        [executorState] { return std::make_unique<FakeTaskExecutor>(executorState); }, {});

    QTRY_VERIFY(runtime.isReady());
    runtime.requestNow(SyncPartition::UserList);
    QTRY_COMPARE(executorState->executionCount.load(), 1);

    runtime.shutdown();
    QTRY_VERIFY(executorState->shutdownRequested.load());
    executorState->shutdownAcknowledgement();
    QTRY_VERIFY(runtime.isStopped());
}

void AdaptiveSyncRuntimeTests::drainsAnActiveExecutorBeforeStopping() {
    const auto executorState = std::make_shared<ExecutorState>();
    AdaptiveSyncRuntime runtime(
        dueRepository,
        [executorState] { return std::make_unique<FakeTaskExecutor>(executorState); }, {});
    QTRY_VERIFY(runtime.isReady());
    runtime.start();
    QTRY_COMPARE(executorState->executionCount.load(), 1);

    runtime.shutdown();
    QTRY_COMPARE(executorState->cancellationCount.load(), 1);
    QTRY_VERIFY(executorState->shutdownRequested.load());
    QVERIFY(!runtime.isStopped());
    executorState->shutdownAcknowledgement();
    QTRY_VERIFY(runtime.isStopped());
}

void AdaptiveSyncRuntimeTests::exposesRepositoryInitializationFailure() {
    const auto executorState = std::make_shared<ExecutorState>();
    AdaptiveSyncRuntime runtime(
        [](QString &error) -> std::unique_ptr<ISyncTaskStateRepository> {
            error = QStringLiteral("task-state database unavailable");
            return {};
        },
        [executorState] { return std::make_unique<FakeTaskExecutor>(executorState); }, {});

    QSignalSpy failed(&runtime, &AdaptiveSyncRuntime::InitializationFailed);
    QTRY_COMPARE(failed.count(), 1);
    QCOMPARE(runtime.initializationError(), QStringLiteral("task-state database unavailable"));
    QVERIFY(!runtime.isReady());
    QTRY_VERIFY(runtime.isStopped());
}

void AdaptiveSyncRuntimeTests::propagatesCompletedBackgroundTask() {
    const auto executorState = std::make_shared<ExecutorState>();
    AdaptiveSyncRuntime runtime(
        dueRepository,
        [executorState] { return std::make_unique<FakeTaskExecutor>(executorState); }, {});
    QSignalSpy completed(&runtime, &AdaptiveSyncRuntime::BackgroundTaskCompleted);

    QTRY_VERIFY(runtime.isReady());
    runtime.start();
    QTRY_COMPARE(executorState->executionCount.load(), 1);

    ISyncTaskExecutor::Completion completion;
    {
        std::lock_guard lock(executorState->completionMutex);
        completion = executorState->completion;
    }
    QVERIFY(completion);
    completion(SyncTaskExecutionResult{.succeeded = true});

    QTRY_COMPARE(completed.count(), 1);
    QCOMPARE(completed.first().first().value<SyncPartition>(), SyncPartition::UserList);

    runtime.shutdown();
    QTRY_VERIFY(executorState->shutdownRequested.load());
    executorState->shutdownAcknowledgement();
    QTRY_VERIFY(runtime.isStopped());
}

void AdaptiveSyncRuntimeTests::defersWorkerDestructionUntilCoordinatorStops() {
    const auto executorState = std::make_shared<ExecutorState>();
    AdaptiveSyncRuntime runtime(
        readyRepository,
        [executorState] { return std::make_unique<FakeTaskExecutor>(executorState); }, {});
    QTRY_VERIFY(runtime.isReady());

    runtime.shutdown();
    QTRY_VERIFY(executorState->shutdownRequested.load());
    QVERIFY(!runtime.isStopped());
    executorState->shutdownAcknowledgement();
    QTRY_VERIFY(runtime.isStopped());
}

void AdaptiveSyncRuntimeTests::destructorRetainsWorkerOwnershipPastLegacyTimeout() {
    const auto executorState = std::make_shared<ExecutorState>();
    auto runtime = std::make_unique<AdaptiveSyncRuntime>(
        readyRepository,
        [executorState] { return std::make_unique<FakeTaskExecutor>(executorState); },
        std::map<SyncTaskKind, SyncSchedulePolicy>{});
    QTRY_VERIFY(runtime->isReady());

    runtime->shutdown();
    QTRY_VERIFY(executorState->shutdownRequested.load());

    std::thread acknowledgeAfterLegacyTimeout([executorState] {
        std::this_thread::sleep_for(std::chrono::milliseconds(5'100));
        executorState->shutdownAcknowledgement();
    });
    QElapsedTimer elapsed;
    elapsed.start();
    runtime.reset();
    acknowledgeAfterLegacyTimeout.join();

    QVERIFY2(elapsed.elapsed() >= 5'000,
             "Runtime destruction released worker ownership before shutdown acknowledgement.");
}

QTEST_GUILESS_MAIN(AdaptiveSyncRuntimeTests)
#include "AdaptiveSyncRuntimeTests.moc"
