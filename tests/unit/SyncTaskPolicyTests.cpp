#include <QtTest>

#include "../../src/application/media/MediaSyncFilter.h"
#include "../../src/application/scheduling/SyncTaskPolicy.h"

class SyncTaskPolicyTests : public QObject {
    Q_OBJECT

private slots:
    void providesStablePersistedNamesForEveryTaskKind();
    void preservesEmptyMediaFilterBehaviorWhileIdentifyingItsPartition();
    void defersFreshCacheUntilItsNormalInterval();
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
