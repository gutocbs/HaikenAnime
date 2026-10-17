#include <QSignalSpy>
#include <QtTest>

#include "../../src/presentation/sync/SynchronizationStatusController.h"

namespace {
SyncProgress progress(const SyncPartition partition, const QString &stageKey, const int processed,
                      const int total) {
    SyncProgress value;
    value.partition = partition;
    value.stageKey = stageKey;
    value.stageIndex = 1;
    value.stageCount = 3;
    value.processedItems = processed;
    value.totalItems = total;
    return value;
}
}

class SynchronizationStatusControllerTests final : public QObject {
    Q_OBJECT

private slots:
    void startsIdleAndHidden();
    void startedShowsRunningStatus();
    void progressExposesStageAndPercent();
    void unknownTotalHasNoKnownProgress();
    void percentIsBoundedTo100();
    void unknownStageHasEmptyLabel();
    void pendingChangesStageHasLabel();
    void completedShowsSuccessThenHides();
    void failedShowsSafeErrorThenHides();
    void failedWithoutMessageUsesFallback();
    void initializationFailureAlwaysShows();
    void staysRunningWhileAnotherPartitionIsActive();
    void staysRunningPastSettleAndHideDelays();
    void failureInRoundWinsOverLaterCompletion();
    void newStartDuringLingerCancelsHiding();
    void ignoresProgressForInactivePartition();
    void ignoresCompletionOfInactivePartition();
    void ignoresUntrackedPartitions();
    void doesNotPublishSuccessBetweenSequentialTasks();
    void failureSurvivesLaterTaskStartedDuringSettle();
    void startDuringLingerAfterFailureBeginsNewRound();
    void cancelledOnlyRoundHidesImmediatelyWithoutOutcome();
    void cancelledPartitionDoesNotFailTheRound();
    void cancelledWhileAnotherPartitionRunsKeepsRunning();
    void progressAfterCancellationIsIgnored();
    void ignoresCancellationOfInactivePartition();
};

void SynchronizationStatusControllerTests::startsIdleAndHidden() {
    SynchronizationStatusController controller(20, 20);

    QVERIFY(!controller.visible());
    QVERIFY(!controller.running());
    QCOMPARE(controller.outcome(), QStringLiteral("idle"));
    QVERIFY(!controller.progressKnown());
}

void SynchronizationStatusControllerTests::startedShowsRunningStatus() {
    SynchronizationStatusController controller(20, 20);
    QSignalSpy changed(&controller, &SynchronizationStatusController::changed);

    controller.HandleStarted(SyncPartition::UserList);

    QVERIFY(controller.visible());
    QVERIFY(controller.running());
    QCOMPARE(controller.outcome(), QStringLiteral("running"));
    QCOMPARE(controller.message(), QStringLiteral("Sincronizando…"));
    QVERIFY(changed.count() >= 1);
}

void SynchronizationStatusControllerTests::progressExposesStageAndPercent() {
    SynchronizationStatusController controller(20, 20);
    controller.HandleStarted(SyncPartition::UserList);

    controller.HandleProgress(progress(SyncPartition::UserList, QStringLiteral("anime"), 25, 100));

    QCOMPARE(controller.stageKey(), QStringLiteral("anime"));
    QCOMPARE(controller.stageLabel(), QStringLiteral("Anime"));
    QCOMPARE(controller.stageIndex(), 1);
    QCOMPARE(controller.stageCount(), 3);
    QCOMPARE(controller.processedItems(), 25);
    QCOMPARE(controller.totalItems(), 100);
    QVERIFY(controller.progressKnown());
    QCOMPARE(controller.percent(), 25);
}

void SynchronizationStatusControllerTests::unknownTotalHasNoKnownProgress() {
    SynchronizationStatusController controller(20, 20);
    controller.HandleStarted(SyncPartition::UserList);

    controller.HandleProgress(progress(SyncPartition::UserList, QStringLiteral("manga"), 12, -1));

    QVERIFY(!controller.progressKnown());
    QCOMPARE(controller.percent(), 0);
    QCOMPARE(controller.stageLabel(), QStringLiteral("Manga"));
}

void SynchronizationStatusControllerTests::percentIsBoundedTo100() {
    SynchronizationStatusController controller(20, 20);
    controller.HandleStarted(SyncPartition::UserList);

    controller.HandleProgress(progress(SyncPartition::UserList, QStringLiteral("reconciliation"), 500, 100));

    QCOMPARE(controller.percent(), 100);
    QCOMPARE(controller.stageLabel(), QStringLiteral("Reconciliação"));
}

void SynchronizationStatusControllerTests::unknownStageHasEmptyLabel() {
    SynchronizationStatusController controller(20, 20);
    controller.HandleStarted(SyncPartition::UserList);

    controller.HandleProgress(progress(SyncPartition::UserList, QStringLiteral("other"), 1, 2));

    QVERIFY(controller.stageLabel().isEmpty());
}

void SynchronizationStatusControllerTests::pendingChangesStageHasLabel() {
    SynchronizationStatusController controller(20, 20);
    controller.HandleStarted(SyncPartition::PendingChanges);

    controller.HandleProgress(progress(SyncPartition::PendingChanges, QStringLiteral("pending_changes"), 1, 4));

    QCOMPARE(controller.stageLabel(), QStringLiteral("Alterações pendentes"));
}

void SynchronizationStatusControllerTests::completedShowsSuccessThenHides() {
    SynchronizationStatusController controller(400, 20);
    controller.HandleStarted(SyncPartition::UserList);

    controller.HandleCompleted(SyncPartition::UserList);

    QTRY_COMPARE(controller.outcome(), QStringLiteral("succeeded"));
    QVERIFY(controller.visible());
    QVERIFY(!controller.running());
    QCOMPARE(controller.message(), QStringLiteral("Sincronização concluída."));
    QTRY_VERIFY(!controller.visible());
    QCOMPARE(controller.outcome(), QStringLiteral("idle"));
}

void SynchronizationStatusControllerTests::failedShowsSafeErrorThenHides() {
    SynchronizationStatusController controller(400, 20);
    controller.HandleStarted(SyncPartition::UserList);

    controller.HandleFailed(SyncPartition::UserList, QStringLiteral("Sem conexão"));

    QTRY_COMPARE(controller.outcome(), QStringLiteral("failed"));
    QCOMPARE(controller.message(), QStringLiteral("Sem conexão"));
    QVERIFY(controller.visible());
    QVERIFY(!controller.running());
    QTRY_VERIFY(!controller.visible());
    QCOMPARE(controller.outcome(), QStringLiteral("idle"));
}

void SynchronizationStatusControllerTests::failedWithoutMessageUsesFallback() {
    SynchronizationStatusController controller(400, 20);
    controller.HandleStarted(SyncPartition::UserList);

    controller.HandleFailed(SyncPartition::UserList, QString());

    QTRY_COMPARE(controller.outcome(), QStringLiteral("failed"));
    QCOMPARE(controller.message(), QStringLiteral("A sincronização falhou."));
}

void SynchronizationStatusControllerTests::initializationFailureAlwaysShows() {
    SynchronizationStatusController controller(400, 20);

    controller.HandleInitializationFailed(QStringLiteral("Agendamento indisponível"));

    QVERIFY(controller.visible());
    QTRY_COMPARE(controller.outcome(), QStringLiteral("failed"));
    QCOMPARE(controller.message(), QStringLiteral("Agendamento indisponível"));
    QTRY_VERIFY(!controller.visible());
}

void SynchronizationStatusControllerTests::staysRunningWhileAnotherPartitionIsActive() {
    SynchronizationStatusController controller(400, 20);
    controller.HandleStarted(SyncPartition::UserList);
    controller.HandleStarted(SyncPartition::PendingChanges);

    controller.HandleCompleted(SyncPartition::UserList);
    QTest::qWait(100);

    QVERIFY(controller.visible());
    QVERIFY(controller.running());
    QCOMPARE(controller.outcome(), QStringLiteral("running"));
    controller.HandleCompleted(SyncPartition::PendingChanges);
    QTRY_COMPARE(controller.outcome(), QStringLiteral("succeeded"));
}

void SynchronizationStatusControllerTests::staysRunningPastSettleAndHideDelays() {
    SynchronizationStatusController controller(20, 20);
    controller.HandleStarted(SyncPartition::UserList);

    QTest::qWait(120);

    QVERIFY(controller.visible());
    QVERIFY(controller.running());
    QCOMPARE(controller.outcome(), QStringLiteral("running"));
}

void SynchronizationStatusControllerTests::failureInRoundWinsOverLaterCompletion() {
    SynchronizationStatusController controller(400, 20);
    controller.HandleStarted(SyncPartition::UserList);
    controller.HandleStarted(SyncPartition::PendingChanges);

    controller.HandleFailed(SyncPartition::UserList, QStringLiteral("Erro de rede"));
    QVERIFY(controller.running());
    QCOMPARE(controller.message(), QStringLiteral("Sincronizando…"));
    controller.HandleCompleted(SyncPartition::PendingChanges);

    QTRY_COMPARE(controller.outcome(), QStringLiteral("failed"));
    QCOMPARE(controller.message(), QStringLiteral("Erro de rede"));
}

void SynchronizationStatusControllerTests::newStartDuringLingerCancelsHiding() {
    SynchronizationStatusController controller(60, 20);
    controller.HandleStarted(SyncPartition::UserList);
    controller.HandleCompleted(SyncPartition::UserList);
    QTRY_COMPARE(controller.outcome(), QStringLiteral("succeeded"));

    controller.HandleStarted(SyncPartition::UserList);
    QTest::qWait(150);

    QVERIFY(controller.visible());
    QVERIFY(controller.running());
    QCOMPARE(controller.outcome(), QStringLiteral("running"));
}

void SynchronizationStatusControllerTests::ignoresProgressForInactivePartition() {
    SynchronizationStatusController controller(20, 20);
    controller.HandleStarted(SyncPartition::UserList);
    controller.HandleProgress(progress(SyncPartition::UserList, QStringLiteral("anime"), 10, 100));
    controller.HandleCompleted(SyncPartition::UserList);
    QSignalSpy changed(&controller, &SynchronizationStatusController::changed);

    controller.HandleProgress(progress(SyncPartition::UserList, QStringLiteral("manga"), 90, 100));
    controller.HandleProgress(progress(SyncPartition::PendingChanges, QStringLiteral("pending_changes"), 1, 2));

    QCOMPARE(changed.count(), 0);
    QCOMPARE(controller.stageKey(), QStringLiteral("anime"));
    QCOMPARE(controller.processedItems(), 10);
}

void SynchronizationStatusControllerTests::ignoresCompletionOfInactivePartition() {
    SynchronizationStatusController controller(20, 20);

    controller.HandleCompleted(SyncPartition::UserList);
    QTest::qWait(60);

    QVERIFY(!controller.visible());
    QCOMPARE(controller.outcome(), QStringLiteral("idle"));
}

void SynchronizationStatusControllerTests::ignoresUntrackedPartitions() {
    SynchronizationStatusController controller(20, 20);

    controller.HandleStarted(SyncPartition::Covers);
    QVERIFY(!controller.visible());
    controller.HandleFailed(SyncPartition::Covers, QStringLiteral("x"));
    QVERIFY(!controller.visible());
    controller.HandleStarted(SyncPartition::ActiveCatalog);
    controller.HandleCompleted(SyncPartition::ActiveCatalog);
    QTest::qWait(80);

    QVERIFY(!controller.visible());
    QVERIFY(!controller.running());
    QCOMPARE(controller.outcome(), QStringLiteral("idle"));
}

void SynchronizationStatusControllerTests::doesNotPublishSuccessBetweenSequentialTasks() {
    SynchronizationStatusController controller(400, 40);
    QStringList outcomes;
    connect(&controller, &SynchronizationStatusController::changed, this,
            [&] { outcomes.append(controller.outcome()); });

    controller.HandleStarted(SyncPartition::UserList);
    controller.HandleCompleted(SyncPartition::UserList);
    QTest::qWait(10);
    controller.HandleStarted(SyncPartition::PendingChanges);
    controller.HandleCompleted(SyncPartition::PendingChanges);

    QVERIFY(!outcomes.contains(QStringLiteral("succeeded")));
    QTRY_COMPARE(controller.outcome(), QStringLiteral("succeeded"));
    QTRY_VERIFY(!controller.visible());
    QCOMPARE(controller.outcome(), QStringLiteral("idle"));
}

void SynchronizationStatusControllerTests::failureSurvivesLaterTaskStartedDuringSettle() {
    SynchronizationStatusController controller(400, 20);
    controller.HandleStarted(SyncPartition::UserList);
    controller.HandleFailed(SyncPartition::UserList, QStringLiteral("Erro de rede"));

    controller.HandleStarted(SyncPartition::PendingChanges);
    controller.HandleCompleted(SyncPartition::PendingChanges);

    QTRY_COMPARE(controller.outcome(), QStringLiteral("failed"));
    QCOMPARE(controller.message(), QStringLiteral("Erro de rede"));
}

void SynchronizationStatusControllerTests::startDuringLingerAfterFailureBeginsNewRound() {
    SynchronizationStatusController controller(500, 10);
    controller.HandleStarted(SyncPartition::UserList);
    controller.HandleFailed(SyncPartition::UserList, QStringLiteral("Erro de rede"));
    QTRY_COMPARE(controller.outcome(), QStringLiteral("failed"));

    controller.HandleStarted(SyncPartition::UserList);
    QCOMPARE(controller.outcome(), QStringLiteral("running"));
    controller.HandleCompleted(SyncPartition::UserList);

    QTRY_COMPARE(controller.outcome(), QStringLiteral("succeeded"));
    QCOMPARE(controller.message(), QStringLiteral("Sincronização concluída."));
}

void SynchronizationStatusControllerTests::cancelledOnlyRoundHidesImmediatelyWithoutOutcome() {
    SynchronizationStatusController controller(400, 200);
    QStringList outcomes;
    connect(&controller, &SynchronizationStatusController::changed, this,
            [&] { outcomes.append(controller.outcome()); });
    controller.HandleStarted(SyncPartition::UserList);

    controller.HandleCancelled(SyncPartition::UserList);

    QVERIFY(!controller.visible());
    QVERIFY(!controller.running());
    QCOMPARE(controller.outcome(), QStringLiteral("idle"));
    QTest::qWait(300);
    QVERIFY(!outcomes.contains(QStringLiteral("succeeded")));
    QVERIFY(!outcomes.contains(QStringLiteral("failed")));
    QVERIFY(!controller.visible());
}

void SynchronizationStatusControllerTests::cancelledPartitionDoesNotFailTheRound() {
    SynchronizationStatusController controller(400, 20);
    controller.HandleStarted(SyncPartition::UserList);
    controller.HandleStarted(SyncPartition::PendingChanges);
    controller.HandleCompleted(SyncPartition::UserList);

    controller.HandleCancelled(SyncPartition::PendingChanges);

    QTRY_COMPARE(controller.outcome(), QStringLiteral("succeeded"));
}

void SynchronizationStatusControllerTests::cancelledWhileAnotherPartitionRunsKeepsRunning() {
    SynchronizationStatusController controller(400, 20);
    controller.HandleStarted(SyncPartition::UserList);
    controller.HandleStarted(SyncPartition::PendingChanges);

    controller.HandleCancelled(SyncPartition::UserList);
    QTest::qWait(100);

    QVERIFY(controller.visible());
    QVERIFY(controller.running());
}

void SynchronizationStatusControllerTests::progressAfterCancellationIsIgnored() {
    SynchronizationStatusController controller(400, 20);
    controller.HandleStarted(SyncPartition::UserList);
    controller.HandleCancelled(SyncPartition::UserList);
    QSignalSpy changed(&controller, &SynchronizationStatusController::changed);

    controller.HandleProgress(progress(SyncPartition::UserList, QStringLiteral("anime"), 5, 10));

    QCOMPARE(changed.count(), 0);
    QVERIFY(!controller.visible());
}

void SynchronizationStatusControllerTests::ignoresCancellationOfInactivePartition() {
    SynchronizationStatusController controller(400, 20);
    QSignalSpy changed(&controller, &SynchronizationStatusController::changed);

    controller.HandleCancelled(SyncPartition::UserList);

    QCOMPARE(changed.count(), 0);
    QVERIFY(!controller.visible());
}

QTEST_GUILESS_MAIN(SynchronizationStatusControllerTests)
#include "SynchronizationStatusControllerTests.moc"
