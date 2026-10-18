#include <QtTest>

#include <limits>

#include "../../src/application/configuration/Settings.h"
#include "../../src/application/media/MediaSyncFilter.h"
#include "../../src/application/scheduling/SyncTaskPolicy.h"

class SyncTaskPolicyTests : public QObject {
    Q_OBJECT

private slots:
    void providesStablePersistedNamesForEveryTaskKind();
    void preservesEmptyMediaFilterBehaviorWhileIdentifyingItsPartition();
    void selectsDistinctDefaultPoliciesForEveryTaskKind();
    void defersFreshCacheUntilItsNormalInterval();
    void marksAgedFreshCacheAsStaleOrExpired();
    void appliesInjectedJitterToNormalIntervals();
    void limitsConsecutiveImmediateRetries();
    void schedulesRetryableFailuresWithBoundedExponentialBackoff();
    void schedulesNonRetryableFailuresAfterCooldown();
    void prioritizesWatchingAndReadingOverInactiveAndCompletedCatalogs();
    void proposesFailureResetAfterSuccess();
    void runsManualRequestsImmediately();
    void runsLocalChangesImmediately();
    void clampsBackwardClockJumpsToANewCadenceFromNow();
    void runsExpiredWorkAfterAForwardClockJump();
    void saturatesInjectedJitterWithoutCreatingImmediateWork();
    void derivesScheduledPriorityFromPartition();
    void appliesDistinctDefaultCadencesAtDecisionLevel();
    void classifiesExactCacheValidityBoundariesAsDue();
};

void SyncTaskPolicyTests::providesStablePersistedNamesForEveryTaskKind() {
    const QList<SyncTaskKind> taskKinds{SyncTaskKind::UserList, SyncTaskKind::PendingChange,
                                        SyncTaskKind::ActiveCatalog, SyncTaskKind::InactiveCatalog,
                                        SyncTaskKind::CompletedCatalog, SyncTaskKind::Cover,
                                        SyncTaskKind::DerivedMetadata};
    const QList<QString> persistedNames{QStringLiteral("user-list"), QStringLiteral("pending-change"),
                                        QStringLiteral("active-catalog"), QStringLiteral("inactive-catalog"),
                                        QStringLiteral("completed-catalog"), QStringLiteral("cover"),
                                        QStringLiteral("derived-metadata")};

    QCOMPARE(taskKinds.size(), persistedNames.size());
    for (qsizetype index = 0; index < taskKinds.size(); ++index) {
        QCOMPARE(ToString(taskKinds.at(index)), persistedNames.at(index));
        QCOMPARE(SyncTaskKindFromString(persistedNames.at(index)), taskKinds.at(index));
    }
}

void SyncTaskPolicyTests::preservesEmptyMediaFilterBehaviorWhileIdentifyingItsPartition() {
    const MediaSyncFilter filter;

    QVERIFY(filter.isEmpty());
    QCOMPARE(filter.partition, SyncPartition::UserList);
    QCOMPARE(ToString(filter.partition), QStringLiteral("user-list"));
}

void SyncTaskPolicyTests::selectsDistinctDefaultPoliciesForEveryTaskKind() {
    const Settings settings;
    const QList<SyncTaskKind> taskKinds{SyncTaskKind::UserList, SyncTaskKind::PendingChange,
                                        SyncTaskKind::ActiveCatalog, SyncTaskKind::InactiveCatalog,
                                        SyncTaskKind::CompletedCatalog, SyncTaskKind::Cover,
                                        SyncTaskKind::DerivedMetadata};
    const QList<std::chrono::milliseconds> intervals{std::chrono::hours(1), std::chrono::minutes(1),
                                                       std::chrono::minutes(30), std::chrono::hours(12),
                                                       std::chrono::hours(24), std::chrono::hours(6),
                                                       std::chrono::hours(2)};
    const QList<std::chrono::milliseconds> ttls{std::chrono::hours(24), std::chrono::hours(1),
                                                  std::chrono::hours(6), std::chrono::hours(24 * 7),
                                                  std::chrono::hours(24 * 14), std::chrono::hours(48),
                                                  std::chrono::hours(12)};

    QCOMPARE(taskKinds.size(), intervals.size());
    QCOMPARE(taskKinds.size(), ttls.size());
    for (qsizetype index = 0; index < taskKinds.size(); ++index) {
        const auto &policy = settings.syncTaskPolicies.at(taskKinds.at(index));
        QCOMPARE(policy.normalInterval, intervals.at(index));
        QCOMPARE(policy.staleProtectionTtl, ttls.at(index));
    }
}

void SyncTaskPolicyTests::defersFreshCacheUntilItsNormalInterval() {
    SyncTaskState state;
    state.kind = SyncTaskKind::UserList;
    state.cacheValidity = CacheValidity::Fresh;
    const auto succeededAt = QDateTime::fromMSecsSinceEpoch(1'000'000, QTimeZone::UTC);
    state.lastSucceededAt = succeededAt;
    const auto now = succeededAt.addMSecs(10'000);
    SyncSchedulePolicy policy;
    policy.normalInterval = std::chrono::minutes(30);

    const auto decision = SyncTaskPolicy::Decide(state, policy, now, {}).schedule;

    QVERIFY(!decision.shouldRunNow);
    QCOMPARE(decision.nextRunAt, succeededAt.addSecs(30 * 60));
}

void SyncTaskPolicyTests::marksAgedFreshCacheAsStaleOrExpired() {
    SyncTaskState state;
    state.kind = SyncTaskKind::UserList;
    state.cacheValidity = CacheValidity::Fresh;
    const auto now = QDateTime::fromMSecsSinceEpoch(3'000'000, QTimeZone::UTC);
    SyncSchedulePolicy policy;
    policy.normalInterval = std::chrono::hours(1);
    policy.staleProtectionTtl = std::chrono::hours(2);

    state.lastSucceededAt = now.addSecs(-90 * 60);
    const auto stale = SyncTaskPolicy::Decide(state, policy, now, {}).schedule;
    QVERIFY(stale.shouldRunNow);
    QCOMPARE(stale.cacheValidity, CacheValidity::Stale);

    state.lastSucceededAt = now.addSecs(-3 * 60 * 60);
    const auto expired = SyncTaskPolicy::Decide(state, policy, now, {}).schedule;
    QVERIFY(expired.shouldRunNow);
    QCOMPARE(expired.cacheValidity, CacheValidity::Expired);
}

void SyncTaskPolicyTests::appliesInjectedJitterToNormalIntervals() {
    SyncTaskState state;
    state.kind = SyncTaskKind::UserList;
    state.cacheValidity = CacheValidity::Fresh;
    const auto succeededAt = QDateTime::fromMSecsSinceEpoch(4'000'000, QTimeZone::UTC);
    state.lastSucceededAt = succeededAt;
    SyncSchedulePolicy policy;
    policy.normalInterval = std::chrono::seconds(8);
    policy.staleProtectionTtl = std::chrono::hours(1);
    policy.jitterRatio = 0.5;

    const auto decision = SyncTaskPolicy::Decide(
        state, policy, succeededAt, {.jitter = std::chrono::milliseconds(500)});

    QVERIFY(!decision.schedule.shouldRunNow);
    QCOMPARE(decision.schedule.nextRunAt, succeededAt.addMSecs(8500));
}

void SyncTaskPolicyTests::limitsConsecutiveImmediateRetries() {
    SyncTaskState state;
    state.kind = SyncTaskKind::PendingChange;
    state.lastErrorCategory = AniListSyncErrorCategory::Network;
    const auto now = QDateTime::fromMSecsSinceEpoch(5'000'000, QTimeZone::UTC);
    SyncSchedulePolicy policy;
    policy.maximumConsecutiveImmediateRetries = 2;
    policy.initialRetryDelay = std::chrono::seconds(5);

    state.consecutiveImmediateRetries = 1;
    const auto immediate = SyncTaskPolicy::Decide(state, policy, now, {}).schedule;
    QVERIFY(immediate.shouldRunNow);
    QCOMPARE(immediate.nextRunAt, now);

    state.consecutiveImmediateRetries = 2;
    const auto delayed = SyncTaskPolicy::Decide(state, policy, now, {}).schedule;
    QVERIFY(!delayed.shouldRunNow);
    QCOMPARE(delayed.nextRunAt, now.addSecs(5));
}

void SyncTaskPolicyTests::schedulesRetryableFailuresWithBoundedExponentialBackoff() {
    SyncTaskState state;
    state.kind = SyncTaskKind::PendingChange;
    state.lastErrorCategory = AniListSyncErrorCategory::Network;
    state.consecutiveFailures = 4;
    const auto now = QDateTime::fromMSecsSinceEpoch(2'000'000, QTimeZone::UTC);
    SyncSchedulePolicy policy;
    policy.initialRetryDelay = std::chrono::seconds(5);
    policy.maximumRetryDelay = std::chrono::seconds(30);

    const auto decision = SyncTaskPolicy::Decide(state, policy, now, {}).schedule;

    QVERIFY(!decision.shouldRunNow);
    QCOMPARE(decision.nextRunAt, now.addSecs(30));
}

void SyncTaskPolicyTests::schedulesNonRetryableFailuresAfterCooldown() {
    SyncTaskState state;
    state.kind = SyncTaskKind::Cover;
    state.lastErrorCategory = AniListSyncErrorCategory::Authorization;
    const auto now = QDateTime::fromMSecsSinceEpoch(3'000'000, QTimeZone::UTC);
    SyncSchedulePolicy policy;
    policy.cooldown = std::chrono::hours(1);

    const auto decision = SyncTaskPolicy::Decide(state, policy, now, {}).schedule;

    QVERIFY(!decision.shouldRunNow);
    QCOMPARE(decision.nextRunAt, now.addSecs(60 * 60));
}

void SyncTaskPolicyTests::prioritizesWatchingAndReadingOverInactiveAndCompletedCatalogs() {
    QVERIFY(SyncTaskPolicy::PriorityFor(SyncPartition::ActiveCatalog)
            > SyncTaskPolicy::PriorityFor(SyncPartition::InactiveCatalog));
    QVERIFY(SyncTaskPolicy::PriorityFor(SyncPartition::ActiveCatalog)
            > SyncTaskPolicy::PriorityFor(SyncPartition::CompletedCatalog));
}

void SyncTaskPolicyTests::proposesFailureResetAfterSuccess() {
    SyncTaskState state;
    state.status = SyncTaskStatus::Succeeded;
    state.lastErrorCategory = AniListSyncErrorCategory::Network;
    state.safeErrorDetail = QStringLiteral("temporary failure");
    state.consecutiveFailures = 3;
    state.consecutiveImmediateRetries = 2;
    const auto now = QDateTime::fromMSecsSinceEpoch(6'000'000, QTimeZone::UTC);

    const auto decision = SyncTaskPolicy::Decide(state, SyncSchedulePolicy{}, now, {});

    QVERIFY(decision.resetFailureState);
    QCOMPARE(decision.proposedState.lastErrorCategory, AniListSyncErrorCategory::None);
    QCOMPARE(decision.proposedState.safeErrorDetail, QString());
    QCOMPARE(decision.proposedState.consecutiveFailures, 0);
    QCOMPARE(decision.proposedState.consecutiveImmediateRetries, 0);
    QVERIFY(decision.schedule.shouldRunNow);
    QCOMPARE(decision.schedule.nextRunAt, now);
}

void SyncTaskPolicyTests::runsManualRequestsImmediately() {
    SyncTaskState state;
    state.kind = SyncTaskKind::Cover;
    state.lastErrorCategory = AniListSyncErrorCategory::Authorization;
    const auto now = QDateTime::fromMSecsSinceEpoch(7'000'000, QTimeZone::UTC);

    const auto decision = SyncTaskPolicy::Decide(
        state, SyncSchedulePolicy{}, now, {.trigger = SyncTaskTrigger::ManualRun});

    QVERIFY(decision.schedule.shouldRunNow);
    QCOMPARE(decision.schedule.nextRunAt, now);
}

void SyncTaskPolicyTests::runsLocalChangesImmediately() {
    SyncTaskState state;
    state.kind = SyncTaskKind::PendingChange;
    state.partition = SyncPartition::PendingChanges;
    state.priority = 1;
    const auto now = QDateTime::fromMSecsSinceEpoch(8'000'000, QTimeZone::UTC);

    const auto decision = SyncTaskPolicy::Decide(
        state, SyncSchedulePolicy{}, now, {.trigger = SyncTaskTrigger::LocalChange});

    QVERIFY(decision.schedule.shouldRunNow);
    QCOMPARE(decision.schedule.nextRunAt, now);
    QCOMPARE(decision.proposedState.priority, SyncTaskPolicy::PriorityFor(SyncPartition::PendingChanges));
}

void SyncTaskPolicyTests::clampsBackwardClockJumpsToANewCadenceFromNow() {
    SyncTaskState state;
    state.kind = SyncTaskKind::UserList;
    state.cacheValidity = CacheValidity::Fresh;
    const auto now = QDateTime::fromMSecsSinceEpoch(10'000'000, QTimeZone::UTC);
    state.lastSucceededAt = now.addSecs(60 * 60);
    SyncSchedulePolicy policy;
    policy.normalInterval = std::chrono::minutes(30);

    const auto decision = SyncTaskPolicy::Decide(state, policy, now, {}).schedule;

    QVERIFY(!decision.shouldRunNow);
    QCOMPARE(decision.nextRunAt, now.addSecs(30 * 60));
}

void SyncTaskPolicyTests::runsExpiredWorkAfterAForwardClockJump() {
    SyncTaskState state;
    state.kind = SyncTaskKind::UserList;
    state.cacheValidity = CacheValidity::Fresh;
    const auto now = QDateTime::fromMSecsSinceEpoch(11'000'000, QTimeZone::UTC);
    state.lastSucceededAt = now.addSecs(-3 * 60 * 60);
    SyncSchedulePolicy policy;
    policy.normalInterval = std::chrono::hours(1);
    policy.staleProtectionTtl = std::chrono::hours(2);

    const auto decision = SyncTaskPolicy::Decide(state, policy, now, {}).schedule;

    QVERIFY(decision.shouldRunNow);
    QCOMPARE(decision.nextRunAt, now);
    QCOMPARE(decision.cacheValidity, CacheValidity::Expired);
}

void SyncTaskPolicyTests::saturatesInjectedJitterWithoutCreatingImmediateWork() {
    SyncTaskState state;
    state.cacheValidity = CacheValidity::Fresh;
    const auto now = QDateTime::fromMSecsSinceEpoch(13'000'000, QTimeZone::UTC);
    state.lastSucceededAt = now;
    SyncSchedulePolicy policy;
    policy.normalInterval = std::chrono::milliseconds::max();
    policy.staleProtectionTtl = std::chrono::milliseconds::max();
    policy.jitterRatio = 1.0;

    const auto decision = SyncTaskPolicy::Decide(
        state, policy, now, {.jitter = std::chrono::milliseconds::max()});

    QVERIFY(!decision.schedule.shouldRunNow);
    QVERIFY(decision.schedule.nextRunAt.isValid());
    QVERIFY(decision.schedule.nextRunAt > now);
}

void SyncTaskPolicyTests::derivesScheduledPriorityFromPartition() {
    SyncTaskState state;
    state.kind = SyncTaskKind::CompletedCatalog;
    state.partition = SyncPartition::CompletedCatalog;
    state.priority = -1;
    const auto now = QDateTime::fromMSecsSinceEpoch(14'000'000, QTimeZone::UTC);
    state.lastSucceededAt = now;
    SyncSchedulePolicy policy;
    policy.normalInterval = std::chrono::hours(1);

    const auto decision = SyncTaskPolicy::Decide(state, policy, now, {});

    QCOMPARE(decision.proposedState.priority, 200);
}

void SyncTaskPolicyTests::appliesDistinctDefaultCadencesAtDecisionLevel() {
    const Settings settings;
    const auto now = QDateTime::fromMSecsSinceEpoch(15'000'000, QTimeZone::UTC);
    const QList<SyncTaskKind> taskKinds{SyncTaskKind::UserList, SyncTaskKind::PendingChange,
                                        SyncTaskKind::ActiveCatalog, SyncTaskKind::InactiveCatalog,
                                        SyncTaskKind::CompletedCatalog, SyncTaskKind::Cover,
                                        SyncTaskKind::DerivedMetadata};
    const QList<SyncPartition> partitions{SyncPartition::UserList, SyncPartition::PendingChanges,
                                          SyncPartition::ActiveCatalog, SyncPartition::InactiveCatalog,
                                          SyncPartition::CompletedCatalog, SyncPartition::Covers,
                                          SyncPartition::DerivedMetadata};
    const QList<qint64> cadenceSeconds{60 * 60, 60, 30 * 60, 12 * 60 * 60, 24 * 60 * 60,
                                       6 * 60 * 60, 2 * 60 * 60};

    QCOMPARE(taskKinds.size(), cadenceSeconds.size());
    QSet<qint64> observedCadences;
    for (qsizetype index = 0; index < taskKinds.size(); ++index) {
        SyncTaskState state;
        state.kind = taskKinds.at(index);
        state.partition = partitions.at(index);
        state.lastSucceededAt = now;

        const auto decision = SyncTaskPolicy::Decide(state, settings.syncTaskPolicies.at(state.kind), now, {});

        QVERIFY(!decision.schedule.shouldRunNow);
        QCOMPARE(decision.schedule.nextRunAt, now.addSecs(cadenceSeconds.at(index)));
        observedCadences.insert(decision.schedule.nextRunAt.toSecsSinceEpoch() - now.toSecsSinceEpoch());
    }
    QCOMPARE(observedCadences.size(), taskKinds.size());
}

void SyncTaskPolicyTests::classifiesExactCacheValidityBoundariesAsDue() {
    SyncTaskState state;
    const auto now = QDateTime::fromMSecsSinceEpoch(16'000'000, QTimeZone::UTC);
    SyncSchedulePolicy policy;
    policy.normalInterval = std::chrono::hours(1);
    policy.staleProtectionTtl = std::chrono::hours(2);

    state.lastSucceededAt = now.addSecs(-60 * 60);
    const auto normalBoundary = SyncTaskPolicy::Decide(state, policy, now, {}).schedule;
    QVERIFY(normalBoundary.shouldRunNow);
    QCOMPARE(normalBoundary.cacheValidity, CacheValidity::Stale);

    state.lastSucceededAt = now.addSecs(-2 * 60 * 60);
    const auto staleBoundary = SyncTaskPolicy::Decide(state, policy, now, {}).schedule;
    QVERIFY(staleBoundary.shouldRunNow);
    QCOMPARE(staleBoundary.cacheValidity, CacheValidity::Expired);
}

QTEST_MAIN(SyncTaskPolicyTests)
#include "SyncTaskPolicyTests.moc"
