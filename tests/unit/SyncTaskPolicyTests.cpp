#include <QtTest>

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
    void honorsRetryAfterWithoutApplyingJitter();
    void marksAgedFreshCacheAsStaleOrExpired();
    void appliesDeterministicJitterToNormalIntervals();
    void limitsConsecutiveImmediateRetries();
    void schedulesRetryableFailuresWithBoundedExponentialBackoff();
    void schedulesNonRetryableFailuresAfterCooldown();
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

    const auto decision = SyncTaskPolicy::Evaluate(state, policy, now, std::nullopt);

    QVERIFY(!decision.shouldRunNow);
    QCOMPARE(decision.nextRunAt, succeededAt.addSecs(30 * 60));
}

void SyncTaskPolicyTests::honorsRetryAfterWithoutApplyingJitter() {
    SyncTaskState state;
    state.kind = SyncTaskKind::PendingChange;
    state.lastErrorCategory = AniListSyncErrorCategory::RateLimit;
    const auto now = QDateTime::fromMSecsSinceEpoch(2'000'000, QTimeZone::UTC);
    const auto retryAfter = now.addSecs(90);
    SyncSchedulePolicy policy;
    policy.jitterRatio = 0.5;

    const auto decision = SyncTaskPolicy::Evaluate(state, policy, now, retryAfter);

    QVERIFY(!decision.shouldRunNow);
    QCOMPARE(decision.nextRunAt, retryAfter);
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
    const auto stale = SyncTaskPolicy::Evaluate(state, policy, now, std::nullopt);
    QVERIFY(stale.shouldRunNow);
    QCOMPARE(stale.cacheValidity, CacheValidity::Stale);

    state.lastSucceededAt = now.addSecs(-3 * 60 * 60);
    const auto expired = SyncTaskPolicy::Evaluate(state, policy, now, std::nullopt);
    QVERIFY(expired.shouldRunNow);
    QCOMPARE(expired.cacheValidity, CacheValidity::Expired);
}

void SyncTaskPolicyTests::appliesDeterministicJitterToNormalIntervals() {
    SyncTaskState state;
    state.kind = SyncTaskKind::UserList;
    state.cacheValidity = CacheValidity::Fresh;
    const auto succeededAt = QDateTime::fromMSecsSinceEpoch(4'000'000, QTimeZone::UTC);
    state.lastSucceededAt = succeededAt;
    SyncSchedulePolicy policy;
    policy.normalInterval = std::chrono::seconds(8);
    policy.staleProtectionTtl = std::chrono::hours(1);
    policy.jitterRatio = 0.5;

    const auto decision = SyncTaskPolicy::Evaluate(state, policy, succeededAt, std::nullopt);

    QVERIFY(!decision.shouldRunNow);
    QCOMPARE(decision.nextRunAt, succeededAt.addMSecs(8500));
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
    const auto immediate = SyncTaskPolicy::Evaluate(state, policy, now, std::nullopt);
    QVERIFY(immediate.shouldRunNow);
    QCOMPARE(immediate.nextRunAt, now);

    state.consecutiveImmediateRetries = 2;
    const auto delayed = SyncTaskPolicy::Evaluate(state, policy, now, std::nullopt);
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

    const auto decision = SyncTaskPolicy::Evaluate(state, policy, now, std::nullopt);

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

    const auto decision = SyncTaskPolicy::Evaluate(state, policy, now, std::nullopt);

    QVERIFY(!decision.shouldRunNow);
    QCOMPARE(decision.nextRunAt, now.addSecs(60 * 60));
}

QTEST_MAIN(SyncTaskPolicyTests)
#include "SyncTaskPolicyTests.moc"
