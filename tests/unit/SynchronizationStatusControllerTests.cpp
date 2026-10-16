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
    void completedShowsSuccessThenHides();
    void failedShowsSafeErrorThenHides();
    void failedWithoutMessageUsesFallback();
    void failureWithoutPriorStartStillShowsAndHides();
    void staysRunningWhileAnotherPartitionIsActive();
    void failureInRoundWinsOverLaterCompletion();
    void newStartDuringHideDelayCancelsHiding();
    void ignoresProgressForInactivePartition();
    void ignoresCompletionOfInactivePartition();
};

void SynchronizationStatusControllerTests::startsIdleAndHidden() {
    SynchronizationStatusController controller(20);

    QVERIFY(!controller.visible());
    QVERIFY(!controller.running());
    QCOMPARE(controller.outcome(), QStringLiteral("idle"));
    QVERIFY(!controller.progressKnown());
}

void SynchronizationStatusControllerTests::startedShowsRunningStatus() {
    SynchronizationStatusController controller(20);
    QSignalSpy changed(&controller, &SynchronizationStatusController::changed);

    controller.HandleStarted(SyncPartition::UserList);

    QVERIFY(controller.visible());
    QVERIFY(controller.running());
    QCOMPARE(controller.outcome(), QStringLiteral("running"));
    QCOMPARE(controller.message(), QStringLiteral("Sincronizando…"));
    QVERIFY(changed.count() >= 1);
}

void SynchronizationStatusControllerTests::progressExposesStageAndPercent() {
    SynchronizationStatusController controller(20);
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
    SynchronizationStatusController controller(20);
    controller.HandleStarted(SyncPartition::UserList);

    controller.HandleProgress(progress(SyncPartition::UserList, QStringLiteral("manga"), 12, -1));

    QVERIFY(!controller.progressKnown());
    QCOMPARE(controller.percent(), 0);
    QCOMPARE(controller.stageLabel(), QStringLiteral("Manga"));
}

void SynchronizationStatusControllerTests::percentIsBoundedTo100() {
    SynchronizationStatusController controller(20);
    controller.HandleStarted(SyncPartition::UserList);

    controller.HandleProgress(progress(SyncPartition::UserList, QStringLiteral("reconciliation"), 500, 100));

    QCOMPARE(controller.percent(), 100);
    QCOMPARE(controller.stageLabel(), QStringLiteral("Reconciliação"));
}

void SynchronizationStatusControllerTests::unknownStageHasEmptyLabel() {
    SynchronizationStatusController controller(20);
    controller.HandleStarted(SyncPartition::UserList);

    controller.HandleProgress(progress(SyncPartition::UserList, QStringLiteral("other"), 1, 2));

    QVERIFY(controller.stageLabel().isEmpty());
}

void SynchronizationStatusControllerTests::completedShowsSuccessThenHides() {
    SynchronizationStatusController controller(20);
    controller.HandleStarted(SyncPartition::UserList);

    controller.HandleCompleted(SyncPartition::UserList);

    QCOMPARE(controller.outcome(), QStringLiteral("succeeded"));
    QVERIFY(controller.visible());
    QVERIFY(!controller.running());
    QCOMPARE(controller.message(), QStringLiteral("Sincronização concluída."));
    QTRY_VERIFY(!controller.visible());
    QCOMPARE(controller.outcome(), QStringLiteral("idle"));
}

void SynchronizationStatusControllerTests::failedShowsSafeErrorThenHides() {
    SynchronizationStatusController controller(20);
    controller.HandleStarted(SyncPartition::UserList);

    controller.HandleFailed(SyncPartition::UserList, QStringLiteral("Sem conexão"));

    QCOMPARE(controller.outcome(), QStringLiteral("failed"));
    QCOMPARE(controller.message(), QStringLiteral("Sem conexão"));
    QVERIFY(controller.visible());
    QVERIFY(!controller.running());
    QTRY_VERIFY(!controller.visible());
    QCOMPARE(controller.outcome(), QStringLiteral("idle"));
}

void SynchronizationStatusControllerTests::failedWithoutMessageUsesFallback() {
    SynchronizationStatusController controller(20);
    controller.HandleStarted(SyncPartition::UserList);

    controller.HandleFailed(SyncPartition::UserList, QString());

    QCOMPARE(controller.message(), QStringLiteral("A sincronização falhou."));
}

void SynchronizationStatusControllerTests::failureWithoutPriorStartStillShowsAndHides() {
    SynchronizationStatusController controller(20);

    controller.HandleFailed(SyncPartition::UserList, QStringLiteral("Falha ao iniciar"));

    QVERIFY(controller.visible());
    QCOMPARE(controller.outcome(), QStringLiteral("failed"));
    QCOMPARE(controller.message(), QStringLiteral("Falha ao iniciar"));
    QTRY_VERIFY(!controller.visible());
}

void SynchronizationStatusControllerTests::staysRunningWhileAnotherPartitionIsActive() {
    SynchronizationStatusController controller(20);
    controller.HandleStarted(SyncPartition::UserList);
    controller.HandleStarted(SyncPartition::PendingChanges);

    controller.HandleCompleted(SyncPartition::UserList);

    QVERIFY(controller.visible());
    QVERIFY(controller.running());
    QCOMPARE(controller.outcome(), QStringLiteral("running"));
    controller.HandleCompleted(SyncPartition::PendingChanges);
    QCOMPARE(controller.outcome(), QStringLiteral("succeeded"));
}

void SynchronizationStatusControllerTests::failureInRoundWinsOverLaterCompletion() {
    SynchronizationStatusController controller(20);
    controller.HandleStarted(SyncPartition::UserList);
    controller.HandleStarted(SyncPartition::PendingChanges);

    controller.HandleFailed(SyncPartition::UserList, QStringLiteral("Erro de rede"));
    QVERIFY(controller.running());
    controller.HandleCompleted(SyncPartition::PendingChanges);

    QCOMPARE(controller.outcome(), QStringLiteral("failed"));
    QCOMPARE(controller.message(), QStringLiteral("Erro de rede"));
}

void SynchronizationStatusControllerTests::newStartDuringHideDelayCancelsHiding() {
    SynchronizationStatusController controller(60);
    controller.HandleStarted(SyncPartition::UserList);
    controller.HandleCompleted(SyncPartition::UserList);

    controller.HandleStarted(SyncPartition::UserList);
    QTest::qWait(120);

    QVERIFY(controller.visible());
    QVERIFY(controller.running());
    QCOMPARE(controller.outcome(), QStringLiteral("running"));
}

void SynchronizationStatusControllerTests::ignoresProgressForInactivePartition() {
    SynchronizationStatusController controller(20);
    controller.HandleStarted(SyncPartition::UserList);
    controller.HandleProgress(progress(SyncPartition::UserList, QStringLiteral("anime"), 10, 100));
    controller.HandleCompleted(SyncPartition::UserList);
    QSignalSpy changed(&controller, &SynchronizationStatusController::changed);

    controller.HandleProgress(progress(SyncPartition::UserList, QStringLiteral("manga"), 90, 100));
    controller.HandleProgress(progress(SyncPartition::PendingChanges, QStringLiteral("pending_changes"), 1, 2));

    QCOMPARE(changed.count(), 0);
    QCOMPARE(controller.stageKey(), QStringLiteral("anime"));
    QCOMPARE(controller.processedItems(), 10);
    QCOMPARE(controller.outcome(), QStringLiteral("succeeded"));
}

void SynchronizationStatusControllerTests::ignoresCompletionOfInactivePartition() {
    SynchronizationStatusController controller(20);

    controller.HandleCompleted(SyncPartition::UserList);

    QVERIFY(!controller.visible());
    QCOMPARE(controller.outcome(), QStringLiteral("idle"));
}

QTEST_GUILESS_MAIN(SynchronizationStatusControllerTests)
#include "SynchronizationStatusControllerTests.moc"
