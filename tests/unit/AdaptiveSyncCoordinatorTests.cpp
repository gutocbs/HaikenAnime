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

    [[nodiscard]] SyncTaskState State(const SyncPartition partition) const {
        for (const auto &state : states) {
            if (state.partition == partition) return state;
        }
        return {};
    }

    QList<SyncTaskState> states;
    bool readSucceeds = true;
    bool upsertSucceeds = true;
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

    void SetProgressReporter(ProgressReporter reporter) override { progressReporter = std::move(reporter); }

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
    ProgressReporter progressReporter;
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
    void runsManualRequestsForFreshPartitionsImmediately();
    void stopCancelsWorkAndInvalidatesItsGeneration();
    void cancelAcknowledgesOnlyAfterTheExecutorConfirmsTheActivePartition();
    void cancelAcknowledgesImmediatelyForPartitionsWithoutActiveWork();
    void cancelEmitsTaskCancelledAndReschedulesRemainingWork();
    void dropsProgressReportedAfterCancellation();
    void ignoresExecutorCallbacksAfterTheCoordinatorIsDestroyed();
    void doesNotExecuteWhenRunningGenerationCannotBePersisted();
    void waitsForCancellationAndExecutorShutdownAcknowledgements();
    void holdsPendingChangesUntilUserListSynchronizationSucceeds();
    void republishesExecutorProgressInOrder();
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

    coordinator.RequestNow(SyncPartition::PendingChanges);
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
    repository.states = {DueState(SyncTaskKind::PendingChange, SyncPartition::PendingChanges),
                         DueState(SyncTaskKind::UserList, SyncPartition::UserList)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });

    coordinator.Start();

    QCOMPARE(executor.started, QList<SyncPartition>({SyncPartition::UserList}));
    executor.Complete(SyncPartition::UserList, {.succeeded = true});
    QTRY_COMPARE(executor.started,
                 QList<SyncPartition>({SyncPartition::UserList, SyncPartition::PendingChanges}));
}

void AdaptiveSyncCoordinatorTests::runsOnlyOneDueTaskAtATime() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList),
                         DueState(SyncTaskKind::PendingChange, SyncPartition::PendingChanges)};
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
    policies.emplace(SyncTaskKind::PendingChange, DefaultSyncSchedulePolicy(SyncTaskKind::PendingChange));
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); }, policies, true);

    QVERIFY(coordinator.Start());

    QCOMPARE(repository.states.size(), 2);
    QCOMPARE(executor.started, QList<SyncPartition>({SyncPartition::UserList}));
}

void AdaptiveSyncCoordinatorTests::doesNotSchedulePersistedPartitionsOutsideConfiguredPolicies() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList),
                         DueState(SyncTaskKind::PendingChange, SyncPartition::PendingChanges)};
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
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList),
                         DueState(SyncTaskKind::PendingChange, SyncPartition::PendingChanges)};
    FakeTaskExecutor executor;
    auto policies = DefaultSyncTaskPolicies();
    policies[SyncTaskKind::PendingChange].maximumConsecutiveImmediateRetries = 0;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); }, policies);

    coordinator.Start();
    executor.Complete(SyncPartition::UserList,
                      {.succeeded = true, .confirmedPage = 3});
    QTRY_VERIFY(executor.started.contains(SyncPartition::PendingChanges));
    executor.Complete(SyncPartition::PendingChanges,
                      {.succeeded = false,
                       .errorCategory = AniListSyncErrorCategory::Network,
                       .safeErrorDetail = QStringLiteral("network unavailable")});

    QTRY_COMPARE(repository.State(SyncPartition::PendingChanges).status, SyncTaskStatus::RetryScheduled);
    QTRY_COMPARE(repository.State(SyncPartition::UserList).status, SyncTaskStatus::Succeeded);
    const auto completed = repository.State(SyncPartition::UserList);
    QCOMPARE(completed.confirmedPage, std::optional<int>(3));
}

void AdaptiveSyncCoordinatorTests::resumesAnIncompleteTaskFromItsPersistedCheckpoint() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    auto interrupted = DueState(SyncTaskKind::UserList, SyncPartition::UserList);
    interrupted.status = SyncTaskStatus::RetryScheduled;
    interrupted.confirmedPage = 2;
    repository.states = {interrupted};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });

    QVERIFY(coordinator.Start());

    const auto resumed = executor.requests.at(SyncPartition::UserList).state;
    QCOMPARE(resumed.confirmedPage, std::optional<int>(2));
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
    state.kind = SyncTaskKind::UserList;
    state.partition = SyncPartition::UserList;
    state.status = SyncTaskStatus::RetryScheduled;
    state.confirmedPage = 4;

    const auto request = SyncTaskRequest::ForState(state);

    QCOMPARE(request.filter.partition, SyncPartition::UserList);
    QCOMPARE(request.filter.startingPage, 5);
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

void AdaptiveSyncCoordinatorTests::runsManualRequestsForFreshPartitionsImmediately() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    auto freshUserList = DueState(SyncTaskKind::UserList, SyncPartition::UserList);
    freshUserList.lastSucceededAt = clock.CurrentTime();
    freshUserList.cacheValidity = CacheValidity::Fresh;
    repository.states = {freshUserList};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });

    coordinator.Start();
    QVERIFY(executor.started.isEmpty());
    coordinator.RequestNow(SyncPartition::UserList);

    QCOMPARE(executor.started, QList<SyncPartition>{SyncPartition::UserList});
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

void AdaptiveSyncCoordinatorTests::cancelAcknowledgesOnlyAfterTheExecutorConfirmsTheActivePartition() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });
    QSignalSpy completed(&coordinator, &AdaptiveSyncCoordinator::TaskCompleted);
    int acknowledged = 0;

    coordinator.Start();
    const auto runningGeneration = repository.State(SyncPartition::UserList).generation;
    coordinator.Cancel(SyncPartition::UserList, [&acknowledged] { ++acknowledged; });

    QVERIFY(executor.cancelled.contains(SyncPartition::UserList));
    QCoreApplication::processEvents();
    QCOMPARE(acknowledged, 0);
    QVERIFY(repository.State(SyncPartition::UserList).generation > runningGeneration);

    const auto staleCompletion = executor.requests.at(SyncPartition::UserList).completion;
    executor.AcknowledgeCancellation(SyncPartition::UserList);
    QTRY_COMPARE(acknowledged, 1);
    // The freed slot restarts the due partition; only the cancelled run's late completion must be ignored.
    staleCompletion({.succeeded = true, .confirmedPage = 1});
    QCoreApplication::processEvents();
    QCOMPARE(completed.count(), 0);
}

void AdaptiveSyncCoordinatorTests::cancelEmitsTaskCancelledAndReschedulesRemainingWork() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList),
                         DueState(SyncTaskKind::PendingChange, SyncPartition::PendingChanges)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });
    QSignalSpy cancelled(&coordinator, &AdaptiveSyncCoordinator::TaskCancelled);
    QSignalSpy completed(&coordinator, &AdaptiveSyncCoordinator::TaskCompleted);
    QSignalSpy failed(&coordinator, &AdaptiveSyncCoordinator::TaskFailed);

    coordinator.Start();
    QCOMPARE(executor.started.count(), 1);
    const auto first = executor.started.constFirst();
    coordinator.Cancel(first, {});
    QCoreApplication::processEvents();
    QCOMPARE(cancelled.count(), 0);

    executor.AcknowledgeCancellation(first);

    QTRY_COMPARE(cancelled.count(), 1);
    QCOMPARE(cancelled.first().first().value<SyncPartition>(), first);
    QCOMPARE(completed.count(), 0);
    QCOMPARE(failed.count(), 0);
    // The freed slot lets the remaining due work run without waiting for another trigger.
    QTRY_COMPARE(executor.started.count(), 2);
}

void AdaptiveSyncCoordinatorTests::dropsProgressReportedAfterCancellation() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });
    QSignalSpy progressSpy(&coordinator, &AdaptiveSyncCoordinator::TaskProgress);

    coordinator.Start();
    coordinator.Cancel(SyncPartition::UserList, {});
    executor.progressReporter({.partition = SyncPartition::UserList, .stageKey = QStringLiteral("anime"),
                               .stageIndex = 1, .stageCount = 3, .processedItems = 4});
    QCoreApplication::processEvents();

    QCOMPARE(progressSpy.count(), 0);
    executor.AcknowledgeCancellation(SyncPartition::UserList);
    QCoreApplication::processEvents();
    QCOMPARE(progressSpy.count(), 0);
}

void AdaptiveSyncCoordinatorTests::cancelAcknowledgesImmediatelyForPartitionsWithoutActiveWork() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor, [&clock] { return clock.CurrentTime(); });
    int acknowledged = 0;

    coordinator.Start();
    coordinator.Cancel(SyncPartition::PendingChanges, [&acknowledged] { ++acknowledged; });

    QCOMPARE(acknowledged, 1);
    QVERIFY(!executor.cancelled.contains(SyncPartition::PendingChanges));
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

void AdaptiveSyncCoordinatorTests::republishesExecutorProgressInOrder() {
    FakeClock clock;
    FakeTaskStateRepository repository;
    repository.states = {DueState(SyncTaskKind::UserList, SyncPartition::UserList)};
    FakeTaskExecutor executor;
    AdaptiveSyncCoordinator coordinator(repository, executor,
        [&clock] { return clock.CurrentTime(); }, DefaultSyncTaskPolicies(), false);
    QSignalSpy progressSpy(&coordinator, &AdaptiveSyncCoordinator::TaskProgress);
    QVERIFY(coordinator.Start());
    QVERIFY(executor.progressReporter);

    executor.progressReporter({.partition = SyncPartition::UserList, .stageKey = QStringLiteral("anime"),
                               .stageIndex = 1, .stageCount = 3, .processedItems = 10});
    executor.progressReporter({.partition = SyncPartition::UserList, .stageKey = QStringLiteral("manga"),
                               .stageIndex = 2, .stageCount = 3, .processedItems = 4});

    QTRY_COMPARE(progressSpy.count(), 2);
    const auto first = progressSpy.at(0).at(0).value<SyncProgress>();
    const auto second = progressSpy.at(1).at(0).value<SyncProgress>();
    QCOMPARE(first.partition, SyncPartition::UserList);
    QCOMPARE(first.stageKey, QStringLiteral("anime"));
    QCOMPARE(first.processedItems, 10);
    QCOMPARE(second.stageKey, QStringLiteral("manga"));
    QCOMPARE(second.stageIndex, 2);
    QCOMPARE(second.totalItems, -1);
}
