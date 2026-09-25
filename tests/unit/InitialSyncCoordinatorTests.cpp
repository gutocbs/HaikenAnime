#include <QSemaphore>
#include <QSignalSpy>
#include <QThread>
#include <QtTest>

#include <atomic>
#include <thread>

#include "../../src/app/InitialSyncCoordinator.h"

namespace {
class SemaphoreReleaseGuard final {
public:
    explicit SemaphoreReleaseGuard(QSemaphore &semaphore)
        : semaphore_(semaphore) {
    }

    ~SemaphoreReleaseGuard() {
        semaphore_.release();
    }

private:
    QSemaphore &semaphore_;
};
}

class InitialSyncCoordinatorTests final : public QObject {
    Q_OBJECT

private slots:
    void doesNotRunConcurrentSynchronizations();
    void shutdownWaitsForActiveSynchronization();
    void shutdownBeforeStartRejectsFutureStart();
    void repeatedShutdownIsSafe();
    void completionDuringShutdownDoesNotRestartScheduler();
    void shutdownFromStartedSignalPreventsWork();
    void recursiveStartFromStartedSignalDoesNotStartTwice();
};

void InitialSyncCoordinatorTests::doesNotRunConcurrentSynchronizations() {
    std::atomic_int calls = 0;
    QSemaphore entered;
    QSemaphore release;
    InitialSyncCoordinator coordinator(
        [&](QString &) {
            ++calls;
            entered.release();
            release.acquire();
            return true;
        },
        60000);
    SemaphoreReleaseGuard releaseGuard(release);

    coordinator.start();
    QVERIFY(entered.tryAcquire(1, 1000));
    coordinator.start();
    QCOMPARE(calls.load(), 1);

    release.release();
    coordinator.shutdown();
}

void InitialSyncCoordinatorTests::shutdownWaitsForActiveSynchronization() {
    std::atomic_bool finished = false;
    QSemaphore entered;
    QSemaphore release;
    InitialSyncCoordinator coordinator(
        [&](QString &) {
            entered.release();
            release.acquire();
            finished = true;
            return true;
        },
        60000);
    SemaphoreReleaseGuard releaseGuard(release);

    coordinator.start();
    QVERIFY(entered.tryAcquire(1, 1000));
    std::thread releaser([&] {
        QThread::msleep(20);
        release.release();
    });

    coordinator.shutdown();
    const bool finishedWhenShutdownReturned = finished.load();
    releaser.join();
    QTRY_VERIFY_WITH_TIMEOUT(finished.load(), 1000);

    QVERIFY(finishedWhenShutdownReturned);
}

void InitialSyncCoordinatorTests::shutdownBeforeStartRejectsFutureStart() {
    std::atomic_int calls = 0;
    InitialSyncCoordinator coordinator([&](QString &) {
        ++calls;
        return true;
    }, 60000);

    coordinator.shutdown();
    coordinator.start();
    QTest::qWait(30);

    QCOMPARE(calls.load(), 0);
}

void InitialSyncCoordinatorTests::repeatedShutdownIsSafe() {
    std::atomic_int calls = 0;
    InitialSyncCoordinator coordinator([&](QString &) {
        ++calls;
        return true;
    }, 60000);
    QSignalSpy completedSpy(&coordinator, &InitialSyncCoordinator::completed);

    coordinator.start();
    QVERIFY(completedSpy.wait(1000));
    coordinator.shutdown();
    coordinator.shutdown();

    QCOMPARE(calls.load(), 1);
}

void InitialSyncCoordinatorTests::completionDuringShutdownDoesNotRestartScheduler() {
    std::atomic_int calls = 0;
    QSemaphore entered;
    QSemaphore release;
    InitialSyncCoordinator coordinator(
        [&](QString &) {
            ++calls;
            entered.release();
            release.acquire();
            return true;
        },
        10);
    SemaphoreReleaseGuard releaseGuard(release);

    coordinator.start();
    QVERIFY(entered.tryAcquire(1, 1000));
    std::thread releaser([&] {
        QThread::msleep(20);
        release.release();
    });

    coordinator.shutdown();
    releaser.join();
    QTest::qWait(40);

    QCOMPARE(calls.load(), 1);
}

void InitialSyncCoordinatorTests::shutdownFromStartedSignalPreventsWork() {
    std::atomic_int calls = 0;
    InitialSyncCoordinator coordinator([&](QString &) {
        ++calls;
        return true;
    }, 60000);
    connect(&coordinator, &InitialSyncCoordinator::started,
            &coordinator, &InitialSyncCoordinator::shutdown,
            Qt::DirectConnection);

    coordinator.start();
    QTest::qWait(30);

    QCOMPARE(calls.load(), 0);
}

void InitialSyncCoordinatorTests::recursiveStartFromStartedSignalDoesNotStartTwice() {
    std::atomic_int calls = 0;
    InitialSyncCoordinator coordinator([&](QString &) {
        ++calls;
        return true;
    }, 60000);
    connect(&coordinator, &InitialSyncCoordinator::started,
            &coordinator, &InitialSyncCoordinator::start,
            Qt::DirectConnection);
    QSignalSpy completedSpy(&coordinator, &InitialSyncCoordinator::completed);

    coordinator.start();
    QVERIFY(completedSpy.wait(1000));
    coordinator.shutdown();

    QCOMPARE(calls.load(), 1);
}

QTEST_MAIN(InitialSyncCoordinatorTests)
#include "InitialSyncCoordinatorTests.moc"
