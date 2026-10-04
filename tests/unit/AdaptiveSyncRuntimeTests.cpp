#include <QElapsedTimer>
#include <QSemaphore>
#include <QSignalSpy>
#include <QtTest>

#include <atomic>
#include <memory>

#include "../../src/app/AdaptiveSyncRuntime.h"

namespace {
class FakeTaskStateRepository final : public ISyncTaskStateRepository {
public:
    bool ReadAll(QList<SyncTaskState> &states, QString &error) override {
        states.clear();
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
};

struct ExecutorState final {
    std::atomic_bool shutdownRequested = false;
    ISyncTaskExecutor::ShutdownAcknowledgement shutdownAcknowledgement;
};

class FakeTaskExecutor final : public ISyncTaskExecutor {
public:
    explicit FakeTaskExecutor(std::shared_ptr<ExecutorState> state) : state_(std::move(state)) {}

    void Execute(const SyncTaskState &, qint64, Completion) override {}
    void Cancel(SyncPartition, CancellationAcknowledgement acknowledgement) override { acknowledgement(); }
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
}

class AdaptiveSyncRuntimeTests final : public QObject {
    Q_OBJECT
private slots:
    void constructorQueuesInitializationWithoutBlockingCaller();
    void exposesRepositoryInitializationFailure();
    void defersWorkerDestructionUntilCoordinatorStops();
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

QTEST_GUILESS_MAIN(AdaptiveSyncRuntimeTests)
#include "AdaptiveSyncRuntimeTests.moc"
