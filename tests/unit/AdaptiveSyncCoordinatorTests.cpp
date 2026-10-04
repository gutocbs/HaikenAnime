#include <QSignalSpy>
#include <QtTest>

#include <functional>
#include <map>

#include "../../src/app/AdaptiveSyncCoordinator.h"

namespace {
struct FakeClock final {
    QDateTime now = QDateTime::fromMSecsSinceEpoch(20'000'000, QTimeZone::UTC);

    [[nodiscard]] QDateTime CurrentTime() const { return now; }
    void Advance(const std::chrono::milliseconds duration) { now = now.addMSecs(duration.count()); }
};

class FakeTaskStateRepository final : public ISyncTaskStateRepository {
public:
    bool ReadAll(QList<SyncTaskState> &result, QString &error) override {
        result = states;
        error.clear();
        return readSucceeds;
    }

    bool Upsert(const SyncTaskState &state, QString &error) override {
        ++upsertCount;
        for (auto &existing : states) {
            if (existing.partition == state.partition) {
                existing = state;
                error.clear();
                return true;
            }
        }
        states.append(state);
        error.clear();
        return true;
    }

    bool Remove(const SyncPartition &partition, QString &error) override {
        for (auto index = states.size() - 1; index >= 0; --index) {
            if (states.at(index).partition == partition) states.removeAt(index);
        }
        error.clear();
        return true;
    }

    [[nodiscard]] SyncTaskState State(const SyncPartition partition) const {
        for (const auto &state : states) {
            if (state.partition == partition) return state;
        }
        return {};
    }

    QList<SyncTaskState> states;
    bool readSucceeds = true;
    int upsertCount = 0;
};

class FakeTaskExecutor final : public ISyncTaskExecutor {
public:
    struct Request final {
        SyncTaskState state;
        qint64 generation = 0;
        Completion completion;
    };

    void Execute(const SyncTaskState &state, const qint64 generation, Completion completion) override {
        started.append(state.partition);
        requests[state.partition] = {.state = state, .generation = generation, .completion = std::move(completion)};
    }

    void Cancel(const SyncPartition partition) override { cancelled.append(partition); }

    void Complete(const SyncPartition partition, const SyncTaskExecutionResult &result) {
        requests.at(partition).completion(result);
    }

    QList<SyncPartition> started;
    QList<SyncPartition> cancelled;
    std::map<SyncPartition, Request> requests;
};

SyncTaskState DueState(const SyncTaskKind kind, const SyncPartition partition) {
    SyncTaskState state;
    state.kind = kind;
    state.partition = partition;
    state.cacheValidity = CacheValidity::Expired;
    return state;
}
}

class AdaptiveSyncCoordinatorTests final : public QObject {
    Q_OBJECT

private slots:
    void selectsDueTasksInPolicyPriorityOrder();
    void neverStartsTheSamePartitionTwiceWhileItIsRunning();
    void recordsIndependentProgressWhenAnotherPartitionFails();
    void schedulesRetryAtThePolicyDueTime();
    void promotesManualAndLocalChangeRequests();
    void stopCancelsWorkAndInvalidatesItsGeneration();
    void ignoresExecutorCallbacksAfterTheCoordinatorIsDestroyed();
};

void AdaptiveSyncCoordinatorTests::selectsDueTasksInPolicyPriorityOrder() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::Cover, SyncPartition::Covers),
                         DueState(SyncTaskKind::UserList, SyncPartition::UserList),
                         DueState(SyncTaskKind::ActiveCatalog, SyncPartition::ActiveCatalog)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });

    coordinator.Start();

    QCOMPARE(executor.started,
             QList<SyncPartition>({SyncPartition::UserList, SyncPartition::ActiveCatalog,
                                   SyncPartition::Covers}));
}

void AdaptiveSyncCoordinatorTests::neverStartsTheSamePartitionTwiceWhileItIsRunning() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });

    coordinator.Start();
    coordinator.RequestNow(SyncPartition::UserList);
    coordinator.ProcessDueTasks();

    QCOMPARE(executor.started, QList<SyncPartition>({SyncPartition::UserList}));
}

void AdaptiveSyncCoordinatorTests::recordsIndependentProgressWhenAnotherPartitionFails() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::PendingChange, SyncPartition::PendingChanges),
                         DueState(SyncTaskKind::ActiveCatalog, SyncPartition::ActiveCatalog)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });

    coordinator.Start();
    executor.Complete(SyncPartition::PendingChanges,
                      {.succeeded = false,
                       .errorCategory = AniListSyncErrorCategory::Network,
                       .safeErrorDetail = QStringLiteral("network unavailable")});
    executor.Complete(SyncPartition::ActiveCatalog,
                      {.succeeded = true, .confirmedPage = 3, .confirmedCursor = QStringLiteral("cursor-3")});

    QTRY_COMPARE(repository.State(SyncPartition::PendingChanges).status, SyncTaskStatus::RetryScheduled);
    const auto completed = repository.State(SyncPartition::ActiveCatalog);
    QCOMPARE(completed.status, SyncTaskStatus::Succeeded);
    QCOMPARE(completed.confirmedPage, std::optional<int>(3));
    QCOMPARE(completed.confirmedCursor, std::optional<QString>(QStringLiteral("cursor-3")));
}

void AdaptiveSyncCoordinatorTests::schedulesRetryAtThePolicyDueTime() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::PendingChange, SyncPartition::PendingChanges)};
    FakeTaskExecutor executor;
    auto policies = DefaultSyncTaskPolicies();
    policies[SyncTaskKind::PendingChange].initialRetryDelay = std::chrono::seconds(5);
    policies[SyncTaskKind::PendingChange].maximumRetryDelay = std::chrono::seconds(5);
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); }, policies);

    coordinator.Start();
    executor.Complete(SyncPartition::PendingChanges,
                      {.succeeded = false, .errorCategory = AniListSyncErrorCategory::Network});
    QTRY_COMPARE(repository.State(SyncPartition::PendingChanges).nextRunAt, clock.CurrentTime().addSecs(5));
    clock.Advance(std::chrono::seconds(4));
    coordinator.ProcessDueTasks();
    QCOMPARE(executor.started.size(), 1);
    clock.Advance(std::chrono::seconds(1));
    coordinator.ProcessDueTasks();
    QCOMPARE(executor.started.size(), 2);
}

void AdaptiveSyncCoordinatorTests::promotesManualAndLocalChangeRequests() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    auto freshCover = DueState(SyncTaskKind::Cover, SyncPartition::Covers);
    freshCover.lastSucceededAt = clock.CurrentTime();
    freshCover.cacheValidity = CacheValidity::Fresh;
    auto completedCatalog = DueState(SyncTaskKind::CompletedCatalog, SyncPartition::CompletedCatalog);
    completedCatalog.lastSucceededAt = clock.CurrentTime();
    completedCatalog.cacheValidity = CacheValidity::Fresh;
    repository.states = {freshCover, completedCatalog};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });

    coordinator.Start();
    QVERIFY(executor.started.isEmpty());
    coordinator.RequestNow(SyncPartition::Covers);
    coordinator.NotifyLocalChange(SyncPartition::CompletedCatalog);

    QVERIFY(executor.started.contains(SyncPartition::Covers));
    QVERIFY(executor.started.contains(SyncPartition::ActiveCatalog));
    const auto promoted = repository.State(SyncPartition::ActiveCatalog);
    QCOMPARE(promoted.kind, SyncTaskKind::ActiveCatalog);
    QCOMPARE(promoted.partition, SyncPartition::ActiveCatalog);
}

void AdaptiveSyncCoordinatorTests::stopCancelsWorkAndInvalidatesItsGeneration() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });
    QSignalSpy completed(&coordinator, &AdaptiveSyncCoordinator::TaskCompleted);

    coordinator.Start();
    const auto runningGeneration = repository.State(SyncPartition::UserList).generation;
    coordinator.Stop();
    executor.Complete(SyncPartition::UserList, {.succeeded = true, .confirmedPage = 1});

    QVERIFY(executor.cancelled.contains(SyncPartition::UserList));
    QVERIFY(repository.State(SyncPartition::UserList).generation > runningGeneration);
    QCOMPARE(completed.count(), 0);
}

void AdaptiveSyncCoordinatorTests::ignoresExecutorCallbacksAfterTheCoordinatorIsDestroyed() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList)};
    FakeTaskExecutor executor;
    auto *coordinator = new AdaptiveSyncCoordinator(repository, executor, [&clock] { return clock.CurrentTime(); });

    coordinator->Start();
    const auto writesBeforeDestruction = repository.upsertCount;
    delete coordinator;
    executor.Complete(SyncPartition::UserList, {.succeeded = true, .confirmedPage = 1});
    QCoreApplication::processEvents();

    QCOMPARE(repository.upsertCount, writesBeforeDestruction + 1);
}

QTEST_MAIN(AdaptiveSyncCoordinatorTests)
#include "AdaptiveSyncCoordinatorTests.moc"
