#include <QSignalSpy>
#include <QtTest>

#include <functional>
#include <map>

#include "../../src/app/AdaptiveSyncCoordinator.h"
#include "../../src/app/SyncTaskRequest.h"

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
        if (!upsertSucceeds) {
            error = QStringLiteral("state persistence rejected");
            return false;
        }
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
        if (!removeSucceeds) {
            error = QStringLiteral("state removal rejected");
            return false;
        }
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
    bool upsertSucceeds = true;
    bool removeSucceeds = true;
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

    void Cancel(const SyncPartition partition, CancellationAcknowledgement acknowledgement) override {
        cancelled.append(partition);
        cancellationAcknowledgements[partition] = std::move(acknowledgement);
    }

    void Shutdown(ShutdownAcknowledgement acknowledgement) override {
        shutdownAcknowledgement = std::move(acknowledgement);
    }

    void Complete(const SyncPartition partition, const SyncTaskExecutionResult &result) {
        requests.at(partition).completion(result);
    }

    void AcknowledgeCancellation(const SyncPartition partition) {
        cancellationAcknowledgements.at(partition)();
    }

    void AcknowledgeShutdown() { shutdownAcknowledgement(); }

    QList<SyncPartition> started;
    QList<SyncPartition> cancelled;
    std::map<SyncPartition, Request> requests;
    std::map<SyncPartition, CancellationAcknowledgement> cancellationAcknowledgements;
    ShutdownAcknowledgement shutdownAcknowledgement;
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
    void runsOnlyOneDueTaskAtATime();
    void rerunsPartitionWhenRequestedDuringActiveExecution();
    void runsAnotherRequestedPartitionAfterTheActiveOneWithoutRepeatingIt();
    void persistsMissingPolicyPartitionsBeforeSchedulingThem();
    void doesNotSchedulePersistedPartitionsOutsideConfiguredPolicies();
    void neverStartsTheSamePartitionTwiceWhileItIsRunning();
    void recordsIndependentProgressWhenAnotherPartitionFails();
    void resumesAnIncompleteTaskFromItsPersistedCheckpoint();
    void restartsPersistedRetryScheduledTaskWithoutWaitingForPreviousProcessCooldown();
    void reschedulesSuccessfulWorkAtItsAdaptivePolicyCadence();
    void turnsPersistedCheckpointIntoTheNextPartitionRequestPage();
    void schedulesRetryAtThePolicyDueTime();
    void promotesManualAndLocalChangeRequests();
    void stopCancelsWorkAndInvalidatesItsGeneration();
    void ignoresExecutorCallbacksAfterTheCoordinatorIsDestroyed();
    void doesNotExecuteWhenRunningGenerationCannotBePersisted();
    void retainsExistingActivePartitionDuringCompletedPromotion();
    void forwardsRetryAfterToTheSchedulingPolicy();
    void waitsForCancellationAndExecutorShutdownAcknowledgements();
    void holdsPendingChangesUntilUserListSynchronizationSucceeds();
};

void AdaptiveSyncCoordinatorTests::holdsPendingChangesUntilUserListSynchronizationSucceeds() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList),
                         DueState(SyncTaskKind::PendingChange, SyncPartition::PendingChanges)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor,
        [&clock] { return clock.CurrentTime(); }, DefaultSyncTaskPolicies(), false);

    QVERIFY(coordinator.Start());
    QCOMPARE(executor.started, QList<SyncPartition>{SyncPartition::UserList});

    executor.Complete(SyncPartition::UserList,
                      {.succeeded = false,
                       .errorCategory = AniListSyncErrorCategory::Network,
                       .safeErrorDetail = QStringLiteral("offline")});
    QCoreApplication::processEvents();
    QCOMPARE(executor.started, QList<SyncPartition>{SyncPartition::UserList});

    coordinator.RequestNow(SyncPartition::UserList);
    QTRY_COMPARE(executor.started.size(), 2);
    QCOMPARE(executor.started.last(), SyncPartition::UserList);
    executor.Complete(SyncPartition::UserList, {.succeeded = true});

    QTRY_COMPARE(executor.started.size(), 3);
    QCOMPARE(executor.started.last(), SyncPartition::PendingChanges);
}

void AdaptiveSyncCoordinatorTests::rerunsPartitionWhenRequestedDuringActiveExecution() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::PendingChange, SyncPartition::PendingChanges)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor,
        [&clock] { return clock.CurrentTime(); }, {}, false);

    QVERIFY(coordinator.Start());
    QCOMPARE(executor.started, QList<SyncPartition>{SyncPartition::PendingChanges});

    coordinator.NotifyLocalChange(SyncPartition::PendingChanges);
    executor.Complete(SyncPartition::PendingChanges,
                      SyncTaskExecutionResult{.succeeded = true});

    QTRY_COMPARE(executor.started.size(), 2);
    QCOMPARE(executor.started.last(), SyncPartition::PendingChanges);
}

void AdaptiveSyncCoordinatorTests::runsAnotherRequestedPartitionAfterTheActiveOneWithoutRepeatingIt() {
    FakeClock clock;
    auto userList = DueState(SyncTaskKind::UserList, SyncPartition::UserList);
    userList.status = SyncTaskStatus::Succeeded;
    userList.cacheValidity = CacheValidity::Fresh;
    userList.lastSucceededAt = clock.CurrentTime();
    userList.nextRunAt = clock.CurrentTime().addDays(1);
    auto pendingChanges = userList;
    pendingChanges.kind = SyncTaskKind::PendingChange;
    pendingChanges.partition = SyncPartition::PendingChanges;
    FakeTaskStateRepository repository;
    repository.states = {userList, pendingChanges};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor,
        [&clock] { return clock.CurrentTime(); }, DefaultSyncTaskPolicies(), false);

    QVERIFY(coordinator.Start());
    coordinator.RequestNow(SyncPartition::UserList);
    coordinator.RequestNow(SyncPartition::PendingChanges);
    QCOMPARE(executor.started, QList<SyncPartition>{SyncPartition::UserList});

    executor.Complete(SyncPartition::UserList,
                      SyncTaskExecutionResult{.succeeded = true});

    QTRY_COMPARE(executor.started.size(), 2);
    QCOMPARE(executor.started.last(), SyncPartition::PendingChanges);
}

void AdaptiveSyncCoordinatorTests::selectsDueTasksInPolicyPriorityOrder() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::Cover, SyncPartition::Covers),
                         DueState(SyncTaskKind::UserList, SyncPartition::UserList),
                         DueState(SyncTaskKind::ActiveCatalog, SyncPartition::ActiveCatalog)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });

    coordinator.Start();

    QCOMPARE(executor.started, QList<SyncPartition>({SyncPartition::UserList}));
    executor.Complete(SyncPartition::UserList, {.succeeded = true});
    QTRY_COMPARE(executor.started,
                 QList<SyncPartition>({SyncPartition::UserList, SyncPartition::ActiveCatalog}));
    executor.Complete(SyncPartition::ActiveCatalog, {.succeeded = true});
    QTRY_COMPARE(executor.started,
                 QList<SyncPartition>({SyncPartition::UserList, SyncPartition::ActiveCatalog,
                                       SyncPartition::Covers}));
}

void AdaptiveSyncCoordinatorTests::runsOnlyOneDueTaskAtATime() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList),
                         DueState(SyncTaskKind::ActiveCatalog, SyncPartition::ActiveCatalog),
                         DueState(SyncTaskKind::CompletedCatalog, SyncPartition::CompletedCatalog)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });

    QVERIFY(coordinator.Start());

    QCOMPARE(executor.started.size(), 1);
    executor.Complete(executor.started.constFirst(), {.succeeded = true});

    QTRY_COMPARE(executor.started.size(), 2);
}

void AdaptiveSyncCoordinatorTests::persistsMissingPolicyPartitionsBeforeSchedulingThem() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    FakeTaskExecutor executor;
    std::map<SyncTaskKind, SyncSchedulePolicy> policies;
    policies.emplace(SyncTaskKind::UserList, DefaultSyncSchedulePolicy(SyncTaskKind::UserList));
    policies.emplace(SyncTaskKind::ActiveCatalog, DefaultSyncSchedulePolicy(SyncTaskKind::ActiveCatalog));
    policies.emplace(SyncTaskKind::CompletedCatalog, DefaultSyncSchedulePolicy(SyncTaskKind::CompletedCatalog));
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); }, policies, true);

    QVERIFY(coordinator.Start());

    QCOMPARE(repository.states.size(), 3);
    QCOMPARE(executor.started, QList<SyncPartition>({SyncPartition::UserList}));
}

void AdaptiveSyncCoordinatorTests::doesNotSchedulePersistedPartitionsOutsideConfiguredPolicies() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList),
                         DueState(SyncTaskKind::ActiveCatalog, SyncPartition::ActiveCatalog)};
    FakeTaskExecutor executor;
    std::map<SyncTaskKind, SyncSchedulePolicy> policies;
    policies.emplace(SyncTaskKind::UserList, DefaultSyncSchedulePolicy(SyncTaskKind::UserList));
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); }, policies);

    QVERIFY(coordinator.Start());

    QCOMPARE(executor.started, QList<SyncPartition>({SyncPartition::UserList}));
    executor.Complete(SyncPartition::UserList, {.succeeded = true});
    QCoreApplication::processEvents();
    QCOMPARE(executor.started, QList<SyncPartition>({SyncPartition::UserList}));
    QCOMPARE(repository.states.size(), 2);
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
    auto policies = DefaultSyncTaskPolicies();
    policies[SyncTaskKind::PendingChange].maximumConsecutiveImmediateRetries = 0;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); }, policies);

    coordinator.Start();
    executor.Complete(SyncPartition::PendingChanges,
                      {.succeeded = false,
                       .errorCategory = AniListSyncErrorCategory::Network,
                       .safeErrorDetail = QStringLiteral("network unavailable")});
    QTRY_VERIFY(executor.started.contains(SyncPartition::ActiveCatalog));
    executor.Complete(SyncPartition::ActiveCatalog,
                      {.succeeded = true, .confirmedPage = 3, .confirmedCursor = QStringLiteral("cursor-3")});

    QTRY_COMPARE(repository.State(SyncPartition::PendingChanges).status, SyncTaskStatus::RetryScheduled);
    QTRY_COMPARE(repository.State(SyncPartition::ActiveCatalog).status, SyncTaskStatus::Succeeded);
    const auto completed = repository.State(SyncPartition::ActiveCatalog);
    QCOMPARE(completed.confirmedPage, std::optional<int>(3));
    QCOMPARE(completed.confirmedCursor, std::optional<QString>(QStringLiteral("cursor-3")));
}

void AdaptiveSyncCoordinatorTests::resumesAnIncompleteTaskFromItsPersistedCheckpoint() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    auto interrupted = DueState(SyncTaskKind::UserList, SyncPartition::UserList);
    interrupted.status = SyncTaskStatus::RetryScheduled;
    interrupted.confirmedPage = 2;
    interrupted.confirmedCursor = QStringLiteral("page-2");
    repository.states = {interrupted};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });

    QVERIFY(coordinator.Start());

    const auto resumed = executor.requests.at(SyncPartition::UserList).state;
    QCOMPARE(resumed.confirmedPage, std::optional<int>(2));
    QCOMPARE(resumed.confirmedCursor, std::optional<QString>(QStringLiteral("page-2")));
}

void AdaptiveSyncCoordinatorTests::restartsPersistedRetryScheduledTaskWithoutWaitingForPreviousProcessCooldown() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    auto delayedRetry = DueState(SyncTaskKind::UserList, SyncPartition::UserList);
    delayedRetry.status = SyncTaskStatus::RetryScheduled;
    delayedRetry.lastAttemptedAt = clock.CurrentTime();
    delayedRetry.nextRunAt = clock.CurrentTime().addSecs(60 * 60);
    delayedRetry.lastErrorCategory = AniListSyncErrorCategory::Persistence;
    delayedRetry.safeErrorDetail = QStringLiteral("database is locked");
    repository.states = {delayedRetry};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });

    QVERIFY(coordinator.Start());

    QCOMPARE(executor.started, QList<SyncPartition>({SyncPartition::UserList}));
    QCOMPARE(executor.requests.at(SyncPartition::UserList).state.confirmedPage, std::optional<int>{});
}

void AdaptiveSyncCoordinatorTests::reschedulesSuccessfulWorkAtItsAdaptivePolicyCadence() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList)};
    FakeTaskExecutor executor;
    auto policies = DefaultSyncTaskPolicies();
    policies[SyncTaskKind::UserList].normalInterval = std::chrono::seconds(5);
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); }, policies);

    QVERIFY(coordinator.Start());
    executor.Complete(SyncPartition::UserList, {.succeeded = true, .confirmedPage = 2});
    QTRY_COMPARE(repository.State(SyncPartition::UserList).nextRunAt, clock.CurrentTime().addSecs(5));

    clock.Advance(std::chrono::seconds(5));
    coordinator.ProcessDueTasks();

    QCOMPARE(executor.started.count(SyncPartition::UserList), 2);
    QVERIFY(!executor.requests.at(SyncPartition::UserList).state.confirmedPage.has_value());
}

void AdaptiveSyncCoordinatorTests::turnsPersistedCheckpointIntoTheNextPartitionRequestPage() {
    SyncTaskState state;
    state.kind = SyncTaskKind::CompletedCatalog;
    state.partition = SyncPartition::CompletedCatalog;
    state.status = SyncTaskStatus::RetryScheduled;
    state.confirmedPage = 4;

    const auto request = SyncTaskRequest::ForState(state);

    QCOMPARE(request.filter.partition, SyncPartition::CompletedCatalog);
    QCOMPARE(request.filter.startingPage, 5);
    QCOMPARE(request.variables.value(QStringLiteral("status")).toString(), QStringLiteral("FINISHED"));
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
    executor.Complete(SyncPartition::Covers, {.succeeded = true});
    QTRY_VERIFY(executor.started.contains(SyncPartition::ActiveCatalog));
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
    QCOMPARE(repository.State(SyncPartition::UserList).generation, runningGeneration + 1);
    executor.AcknowledgeCancellation(SyncPartition::UserList);
    executor.AcknowledgeShutdown();
    QTRY_VERIFY(executor.cancelled.contains(SyncPartition::UserList));
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
    executor.AcknowledgeCancellation(SyncPartition::UserList);
    executor.AcknowledgeShutdown();
    executor.Complete(SyncPartition::UserList, {.succeeded = true, .confirmedPage = 1});
    QCoreApplication::processEvents();

    QCOMPARE(repository.upsertCount, writesBeforeDestruction + 1);
}

void AdaptiveSyncCoordinatorTests::doesNotExecuteWhenRunningGenerationCannotBePersisted() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList)};
    repository.upsertSucceeds = false;
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });
    QSignalSpy failure(&coordinator, &AdaptiveSyncCoordinator::SchedulingFailed);

    coordinator.Start();

    QCOMPARE(executor.started.size(), 0);
    QVERIFY(!failure.isEmpty());
}

void AdaptiveSyncCoordinatorTests::retainsExistingActivePartitionDuringCompletedPromotion() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    auto completed = DueState(SyncTaskKind::CompletedCatalog, SyncPartition::CompletedCatalog);
    completed.lastSucceededAt = clock.CurrentTime();
    completed.cacheValidity = CacheValidity::Fresh;
    auto active = DueState(SyncTaskKind::ActiveCatalog, SyncPartition::ActiveCatalog);
    repository.states = {completed, active};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });

    coordinator.Start();
    const auto activeGeneration = repository.State(SyncPartition::ActiveCatalog).generation;
    coordinator.NotifyLocalChange(SyncPartition::CompletedCatalog);
    executor.Complete(SyncPartition::ActiveCatalog, {.succeeded = true, .confirmedPage = 1});

    QTRY_COMPARE(repository.State(SyncPartition::ActiveCatalog).status, SyncTaskStatus::Succeeded);
    QCOMPARE(repository.State(SyncPartition::ActiveCatalog).generation, activeGeneration);
    QCOMPARE(executor.started.count(SyncPartition::ActiveCatalog), 1);
}

void AdaptiveSyncCoordinatorTests::forwardsRetryAfterToTheSchedulingPolicy() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::PendingChange, SyncPartition::PendingChanges)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });

    coordinator.Start();
    const auto retryAfter = clock.CurrentTime().addSecs(90);
    executor.Complete(SyncPartition::PendingChanges,
                      {.succeeded = false, .errorCategory = AniListSyncErrorCategory::RateLimit,
                       .retryAfter = retryAfter});

    QTRY_COMPARE(repository.State(SyncPartition::PendingChanges).nextRunAt, retryAfter);
}

void AdaptiveSyncCoordinatorTests::waitsForCancellationAndExecutorShutdownAcknowledgements() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });
    QSignalSpy stopped(&coordinator, &AdaptiveSyncCoordinator::Stopped);

    coordinator.Start();
    coordinator.Stop();
    QCOMPARE(stopped.count(), 0);
    executor.AcknowledgeCancellation(SyncPartition::UserList);
    QCOMPARE(stopped.count(), 0);
    executor.AcknowledgeShutdown();

    QTRY_COMPARE(stopped.count(), 1);
}

QTEST_MAIN(AdaptiveSyncCoordinatorTests)
#include "AdaptiveSyncCoordinatorTests.moc"
