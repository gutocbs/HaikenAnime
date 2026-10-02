#include <QDir>
#include <QFile>
#include <QSemaphore>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThread>
#include <QtTest>
#include <atomic>
#include <memory>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

#include "../../src/app/LocalLibraryScanCoordinator.h"
#include "../../src/infrastructure/logging/AsyncLogger.h"
#include "../../src/infrastructure/library/LocalLibraryScanner.h"
#include "../../src/infrastructure/library/QtDirectoryEnumerator.h"

namespace {
struct State {
    QThread *scannerCreated = nullptr;
    QThread *repositoryCreated = nullptr;
    QThread *scannerDestroyed = nullptr;
    QThread *repositoryDestroyed = nullptr;
    QThread::Priority priority = QThread::InheritPriority;
    int beginCalls = 0;
    int batchCalls = 0;
    int completeCalls = 0;
    int failCalls = 0;
    qsizetype count = -1;
    LibraryScanStatus status = LibraryScanStatus::Running;
    QString root;
    QString diagnostic;
    bool batchFails = false;
    bool beginFails = false;
    bool completeFails = false;
    bool failFails = false;
    std::function<void()> onBegin;
};

// This fake isolates repository I/O while recording the coordinator's required
// persistence protocol. Scanner traversal is real in the disappearing-root case.
class Repository final : public ILocalFileRepository {
public:
    explicit Repository(State &state) : state_(state) { state_.repositoryCreated = QThread::currentThread(); }
    ~Repository() override { state_.repositoryDestroyed = QThread::currentThread(); }
    bool beginScan(const QString &root, qint64 &id, QString &error) override {
        ++state_.beginCalls;
        state_.root = root;
        id = 42;
        if (state_.onBegin) state_.onBegin();
        if (state_.beginFails) { error = QStringLiteral("begin rejected"); return false; }
        return true;
    }
    bool upsertBatch(qint64 id, const QList<LocalFileObservation> &, QString &error) override {
        if (id != 42) qFatal("Wrong scan ID");
        ++state_.batchCalls;
        if (state_.batchFails) { error = QStringLiteral("batch rejected"); return false; }
        return true;
    }
    bool completeScan(qint64 id, qsizetype count, QString &error) override {
        if (id != 42) qFatal("Wrong scan ID");
        ++state_.completeCalls;
        state_.count = count;
        if (state_.completeFails) { error = QStringLiteral("completion rejected"); return false; }
        return true;
    }
    bool failScan(qint64 id, LibraryScanStatus status, qsizetype count,
                  const QString &diagnostic, QString &error) override {
        if (id != 42) qFatal("Wrong scan ID");
        ++state_.failCalls;
        state_.status = status;
        state_.count = count;
        state_.diagnostic = diagnostic;
        if (state_.failFails) { error = QStringLiteral("failure recording rejected"); return false; }
        return true;
    }
private:
    State &state_;
};

using Scan = std::function<LocalLibraryScanResult(
    const std::function<bool(const QList<LocalFileObservation> &, QString &)> &,
    const std::function<void(const LocalLibraryScanProgress &)> &,
    const std::function<bool()> &)>;

class Scanner final : public ILocalLibraryScanner {
public:
    Scanner(State &state, Scan scan) : state_(state), scan_(std::move(scan)) {
        state_.scannerCreated = QThread::currentThread();
    }
    ~Scanner() override { state_.scannerDestroyed = QThread::currentThread(); }
    LocalLibraryScanResult scan(const LocalLibraryScanRequest &,
        const std::function<bool(const QList<LocalFileObservation> &, QString &)> &consumer,
        const std::function<void(const LocalLibraryScanProgress &)> &progress,
        const std::function<bool()> &stop) override {
        state_.priority = QThread::currentThread()->priority();
        return scan_(consumer, progress, stop);
    }
private:
    State &state_;
    Scan scan_;
};

LocalLibraryScanCoordinator::ScannerFactory scannerFactory(State &state, Scan scan) {
    return [&state, scan = std::move(scan)](QString &) { return std::make_unique<Scanner>(state, scan); };
}
LocalLibraryScanCoordinator::RepositoryFactory repositoryFactory(State &state) {
    return [&state](QString &) { return std::make_unique<Repository>(state); };
}
LocalLibraryScanRequest request() { return {QStringLiteral("Q:/"), {QStringLiteral(".mkv")}}; }
Scan success() { return [](const auto &, const auto &, const auto &) { return LocalLibraryScanResult{true, false, 3, {}}; }; }
QString scanLogPath() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(
        QStringLiteral("logs/haikenanime-%1.log").arg(QDate::currentDate().toString(Qt::ISODate)));
}
QString loggedScanLifecycle(AsyncLogger &logger, LocalLibraryScanCoordinator &coordinator) {
    logger.stop();
    QFile file(scanLogPath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QString::fromUtf8(file.readAll());
}
}

class LocalLibraryScanCoordinatorTests final : public QObject {
    Q_OBJECT
private slots:
    void successfulLifecycleOwnsDependenciesOnLowPriorityWorker();
    void coalescesRapidProgressAndDeliversFinalCount();
    void rejectsConcurrentStartAndCanStartAfterCompletion();
    void scannerFailureNeverCompletes();
    void batchFailureNeverCompletesEvenIfScannerClaimsSuccess();
    void rootDisappearingAfterStartNeverCompletes();
    void listingFailureAfterObservationNeverReconciles();
    void shutdownInterruptsAndWaitsForResourceDestruction();
    void beginFailureDoesNotFinalizeAnUnknownScan();
    void completionFailureRecordsFailedScan();
    void failureRecordingErrorIsVisible();
    void factoryFailureIsVisible();
    void logsSuccessfulLifecycleWithTotals();
    void logsZeroResultAsWarning();
    void logsEnumerationAndRepositoryErrors();
    void logsInterruptedShutdown();
};

void LocalLibraryScanCoordinatorTests::successfulLifecycleOwnsDependenciesOnLowPriorityWorker() {
    State state;
    LocalLibraryScanCoordinator coordinator(scannerFactory(state, success()), repositoryFactory(state));
    QSignalSpy started(&coordinator, &LocalLibraryScanCoordinator::started);
    QSignalSpy completed(&coordinator, &LocalLibraryScanCoordinator::completed);
    QSignalSpy failed(&coordinator, &LocalLibraryScanCoordinator::failed);
    QThread *signalThread = nullptr;
    connect(&coordinator, &LocalLibraryScanCoordinator::completed, &coordinator,
            [&] { signalThread = QThread::currentThread(); });
    QVERIFY(coordinator.start(request()));
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 3000);
    coordinator.shutdown();
    QCOMPARE(started.count(), 1);
    QCOMPARE(started.first().first().toString(), QStringLiteral("Q:/"));
    QCOMPARE(completed.first().first().toLongLong(), 3);
    QCOMPARE(failed.count(), 0);
    QCOMPARE(state.beginCalls, 1);
    QCOMPARE(state.completeCalls, 1);
    QCOMPARE(state.failCalls, 0);
    QCOMPARE(state.root, QStringLiteral("Q:/"));
    QCOMPARE(state.count, 3);
    QVERIFY(state.scannerCreated != QThread::currentThread());
    QCOMPARE(state.scannerCreated, state.repositoryCreated);
    QCOMPARE(state.scannerDestroyed, state.scannerCreated);
    QCOMPARE(state.repositoryDestroyed, state.repositoryCreated);
    QCOMPARE(state.priority, QThread::LowPriority);
    QCOMPARE(signalThread, QThread::currentThread());
}

void LocalLibraryScanCoordinatorTests::coalescesRapidProgressAndDeliversFinalCount() {
    State state;
    LocalLibraryScanCoordinator coordinator(scannerFactory(state, [](const auto &, const auto &progress, const auto &) {
        for (int count = 1; count <= 1000; ++count) progress({count, count});
        return LocalLibraryScanResult{true, false, 1000, {}};
    }), repositoryFactory(state));
    QSignalSpy progress(&coordinator, &LocalLibraryScanCoordinator::progressChanged);
    QSignalSpy completed(&coordinator, &LocalLibraryScanCoordinator::completed);
    QVERIFY(coordinator.start(request()));
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 3000);
    QVERIFY(!progress.isEmpty());
    QVERIFY(progress.count() <= 2);
    QCOMPARE(progress.last().first().toLongLong(), 1000);
}

void LocalLibraryScanCoordinatorTests::rejectsConcurrentStartAndCanStartAfterCompletion() {
    State state;
    QSemaphore entered;
    LocalLibraryScanCoordinator coordinator(scannerFactory(state, [&](const auto &, const auto &, const auto &stop) {
        entered.release();
        while (!stop()) QThread::msleep(1);
        return LocalLibraryScanResult{false, true, 0, QStringLiteral("stopped")};
    }), repositoryFactory(state));
    QVERIFY(coordinator.start(request()));
    QVERIFY(entered.tryAcquire(1, 3000));
    QVERIFY(!coordinator.start(request()));
    coordinator.shutdown();
    QCOMPARE(state.beginCalls, 1);
    QVERIFY(!coordinator.start(request()));
    State next;
    LocalLibraryScanCoordinator reusable(scannerFactory(next, success()), repositoryFactory(next));
    QSignalSpy completed(&reusable, &LocalLibraryScanCoordinator::completed);
    QVERIFY(reusable.start(request()));
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 3000);
    QVERIFY(reusable.start(request()));
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 2, 3000);
    QCOMPARE(next.beginCalls, 2);
}

void LocalLibraryScanCoordinatorTests::scannerFailureNeverCompletes() {
    State state;
    LocalLibraryScanCoordinator coordinator(scannerFactory(state, [](const auto &, const auto &, const auto &) {
        return LocalLibraryScanResult{false, false, 2, QStringLiteral("unreadable child")};
    }), repositoryFactory(state));
    QSignalSpy failed(&coordinator, &LocalLibraryScanCoordinator::failed);
    QSignalSpy completed(&coordinator, &LocalLibraryScanCoordinator::completed);
    QVERIFY(coordinator.start(request()));
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
    QCOMPARE(state.failCalls, 1);
    QCOMPARE(state.completeCalls, 0);
    QCOMPARE(state.status, LibraryScanStatus::Failed);
    QCOMPARE(state.count, 2);
    QCOMPARE(state.diagnostic, QStringLiteral("unreadable child"));
    QCOMPARE(completed.count(), 0);
}

void LocalLibraryScanCoordinatorTests::batchFailureNeverCompletesEvenIfScannerClaimsSuccess() {
    State state;
    state.batchFails = true;
    LocalLibraryScanCoordinator coordinator(scannerFactory(state, [](const auto &consumer, const auto &, const auto &) {
        QString error;
        consumer({LocalFileObservation{}}, error);
        return LocalLibraryScanResult{true, false, 1, {}};
    }), repositoryFactory(state));
    QSignalSpy failed(&coordinator, &LocalLibraryScanCoordinator::failed);
    QVERIFY(coordinator.start(request()));
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
    QCOMPARE(state.batchCalls, 1);
    QCOMPARE(state.failCalls, 1);
    QCOMPARE(state.completeCalls, 0);
    QCOMPARE(state.status, LibraryScanStatus::Failed);
    QVERIFY(failed.first().first().toString().contains(QStringLiteral("batch rejected")));
}

void LocalLibraryScanCoordinatorTests::rootDisappearingAfterStartNeverCompletes() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto root = directory.filePath(QStringLiteral("root"));
    QVERIFY(QDir().mkpath(root));
    State state;
    std::atomic_bool removed = false;
    state.onBegin = [&] { removed = QDir(root).removeRecursively(); };
    // The enumerator outlives the scanner and is created inside the worker.
    class OwnedScanner final : public ILocalLibraryScanner {
    public:
        LocalLibraryScanResult scan(const LocalLibraryScanRequest &request,
            const std::function<bool(const QList<LocalFileObservation> &, QString &)> &consumer,
            const std::function<void(const LocalLibraryScanProgress &)> &progress,
            const std::function<bool()> &stop) override {
            LocalLibraryScanner scanner(enumerator_);
            return scanner.scan(request, consumer, progress, stop);
        }
    private:
        QtDirectoryEnumerator enumerator_;
    };
    LocalLibraryScanCoordinator coordinator([](QString &) { return std::make_unique<OwnedScanner>(); }, repositoryFactory(state));
    QSignalSpy failed(&coordinator, &LocalLibraryScanCoordinator::failed);
    QVERIFY(coordinator.start({root, {QStringLiteral(".mkv")}}));
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
    QVERIFY(removed.load());
    QCOMPARE(state.completeCalls, 0);
    QCOMPARE(state.failCalls, 1);
    QCOMPARE(state.status, LibraryScanStatus::Failed);
    QVERIFY(failed.first().first().toString().contains(root));
}

void LocalLibraryScanCoordinatorTests::listingFailureAfterObservationNeverReconciles() {
#ifdef Q_OS_WIN
    QTemporaryDir root;
    QFile file(root.filePath("observed.mkv"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    class Listing final : public IDirectoryListing {
    public:
        DirectoryListingEntry next() override {
            if (delivered_) return {{}, ERROR_ACCESS_DENIED};
            delivered_ = true;
            return {QStringLiteral("observed.mkv"), 0};
        }
    private:
        bool delivered_ = false;
    };
    class OwnedScanner final : public ILocalLibraryScanner {
    public:
        LocalLibraryScanResult scan(const LocalLibraryScanRequest &request,
            const std::function<bool(const QList<LocalFileObservation> &, QString &)> &consumer,
            const std::function<void(const LocalLibraryScanProgress &)> &progress,
            const std::function<bool()> &stop) override {
            QtDirectoryEnumerator enumerator([](const QString &) { return std::make_unique<Listing>(); });
            LocalLibraryScanner scanner(enumerator);
            return scanner.scan(request, consumer, progress, stop);
        }
    };
    State state;
    LocalLibraryScanCoordinator coordinator([](QString &) { return std::make_unique<OwnedScanner>(); },
                                             repositoryFactory(state));
    QSignalSpy failed(&coordinator, &LocalLibraryScanCoordinator::failed);
    QSignalSpy completed(&coordinator, &LocalLibraryScanCoordinator::completed);
    QVERIFY(coordinator.start({root.path(), {QStringLiteral(".mkv")}, 1, 0}));
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
    coordinator.shutdown();
    QCOMPARE(state.batchCalls, 1);
    QCOMPARE(state.completeCalls, 0);
    QCOMPARE(state.failCalls, 1);
    QCOMPARE(state.count, 1);
    QCOMPARE(state.status, LibraryScanStatus::Failed);
    QCOMPARE(completed.count(), 0);
    QVERIFY(state.diagnostic.contains(root.path()));
#else
    QSKIP("Windows listing errors are platform-specific");
#endif
}

void LocalLibraryScanCoordinatorTests::shutdownInterruptsAndWaitsForResourceDestruction() {
    State state;
    QSemaphore entered;
    LocalLibraryScanCoordinator coordinator(scannerFactory(state, [&](const auto &consumer, const auto &, const auto &stop) {
        QString error;
        consumer({LocalFileObservation{}}, error);
        entered.release();
        while (!stop()) QThread::msleep(1);
        return LocalLibraryScanResult{false, true, 1, QStringLiteral("interrupted")};
    }), repositoryFactory(state));
    QSignalSpy failed(&coordinator, &LocalLibraryScanCoordinator::failed);
    QSignalSpy completed(&coordinator, &LocalLibraryScanCoordinator::completed);
    QVERIFY(coordinator.start(request()));
    QVERIFY(entered.tryAcquire(1, 3000));
    coordinator.shutdown();
    QCOMPARE(state.failCalls, 1);
    QCOMPARE(state.status, LibraryScanStatus::Interrupted);
    QCOMPARE(state.count, 1);
    QCOMPARE(state.batchCalls, 1);
    QCOMPARE(state.completeCalls, 0);
    QVERIFY(state.scannerDestroyed);
    QVERIFY(state.repositoryDestroyed);
    coordinator.shutdown();
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
    QCOMPARE(completed.count(), 0);
    QVERIFY(!coordinator.start(request()));
}

void LocalLibraryScanCoordinatorTests::beginFailureDoesNotFinalizeAnUnknownScan() {
    State state;
    state.beginFails = true;
    LocalLibraryScanCoordinator coordinator(scannerFactory(state, success()), repositoryFactory(state));
    QSignalSpy failed(&coordinator, &LocalLibraryScanCoordinator::failed);
    QVERIFY(coordinator.start(request()));
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
    QCOMPARE(state.completeCalls, 0);
    QCOMPARE(state.failCalls, 0);
    QVERIFY(failed.first().first().toString().contains(QStringLiteral("begin rejected")));
}

void LocalLibraryScanCoordinatorTests::completionFailureRecordsFailedScan() {
    State state;
    state.completeFails = true;
    LocalLibraryScanCoordinator coordinator(scannerFactory(state, success()), repositoryFactory(state));
    QSignalSpy failed(&coordinator, &LocalLibraryScanCoordinator::failed);
    QVERIFY(coordinator.start(request()));
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
    QCOMPARE(state.completeCalls, 1);
    QCOMPARE(state.failCalls, 1);
    QCOMPARE(state.status, LibraryScanStatus::Failed);
    QVERIFY(failed.first().first().toString().contains(QStringLiteral("completion rejected")));
}

void LocalLibraryScanCoordinatorTests::failureRecordingErrorIsVisible() {
    State state;
    state.failFails = true;
    LocalLibraryScanCoordinator coordinator(scannerFactory(state, [](const auto &, const auto &, const auto &) {
        return LocalLibraryScanResult{false, false, 0, QStringLiteral("unreadable")};
    }), repositoryFactory(state));
    QSignalSpy failed(&coordinator, &LocalLibraryScanCoordinator::failed);
    QVERIFY(coordinator.start(request()));
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
    const auto error = failed.first().first().toString();
    QVERIFY(error.contains(QStringLiteral("unreadable")));
    QVERIFY(error.contains(QStringLiteral("failure recording rejected")));
    QCOMPARE(state.completeCalls, 0);
}

void LocalLibraryScanCoordinatorTests::factoryFailureIsVisible() {
    State state;
    LocalLibraryScanCoordinator coordinator(scannerFactory(state, success()), [](QString &error) -> std::unique_ptr<ILocalFileRepository> {
        error = QStringLiteral("database unavailable");
        return {};
    });
    QSignalSpy failed(&coordinator, &LocalLibraryScanCoordinator::failed);
    QVERIFY(coordinator.start(request()));
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
    QVERIFY(failed.first().first().toString().contains(QStringLiteral("database unavailable")));
    QCOMPARE(state.completeCalls, 0);
}

void LocalLibraryScanCoordinatorTests::logsSuccessfulLifecycleWithTotals() {
    QStandardPaths::setTestModeEnabled(true);
    QFile::remove(scanLogPath());
    State state;
    AsyncLogger logger;
    logger.start();
    LocalLibraryScanCoordinator coordinator(scannerFactory(state, [](const auto &, const auto &, const auto &) {
        return LocalLibraryScanResult{true, false, 3, {}, 2};
    }), repositoryFactory(state));
    coordinator.setLogger(&logger);
    QSignalSpy completed(&coordinator, &LocalLibraryScanCoordinator::completed);
    QVERIFY(coordinator.start(request()));
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 3000);
    const auto entries = loggedScanLifecycle(logger, coordinator);
    QVERIFY(entries.contains(QStringLiteral("[LocalLibrary] Local library scan started: root=Q:/")));
    QVERIFY(entries.contains(QStringLiteral("[LocalLibrary] Local library scan completed: outcome=succeeded, elapsedMs=")));
    QVERIFY(entries.contains(QStringLiteral("candidateFiles=3")));
    QVERIFY(entries.contains(QStringLiteral("skippedFiles=2")));
    QCOMPARE(entries.count(QStringLiteral("Local library scan started:")), 1);
    QCOMPARE(entries.count(QStringLiteral("Local library scan completed:")), 1);
    QCOMPARE(entries.count(QStringLiteral("[LocalLibrary]")), 2);
    QVERIFY(!entries.contains(QStringLiteral("Local library file scanned")));
}

void LocalLibraryScanCoordinatorTests::logsZeroResultAsWarning() {
    QStandardPaths::setTestModeEnabled(true);
    QFile::remove(scanLogPath());
    State state;
    AsyncLogger logger;
    logger.start();
    LocalLibraryScanCoordinator coordinator(scannerFactory(state, [](const auto &, const auto &, const auto &) {
        return LocalLibraryScanResult{true, false, 0, {}};
    }), repositoryFactory(state));
    coordinator.setLogger(&logger);
    QSignalSpy completed(&coordinator, &LocalLibraryScanCoordinator::completed);
    QVERIFY(coordinator.start(request()));
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 3000);
    const auto entries = loggedScanLifecycle(logger, coordinator);
    QVERIFY(entries.contains(QStringLiteral("[WARN] [LocalLibrary] Local library scan completed: outcome=zero-results, elapsedMs=")));
    QVERIFY(entries.contains(QStringLiteral("candidateFiles=0")));
    QVERIFY(entries.contains(QStringLiteral("skippedFiles=0")));
}

void LocalLibraryScanCoordinatorTests::logsEnumerationAndRepositoryErrors() {
    QStandardPaths::setTestModeEnabled(true);
    {
        QFile::remove(scanLogPath());
        State state;
        AsyncLogger logger;
        logger.start();
        LocalLibraryScanCoordinator coordinator(scannerFactory(state, [](const auto &, const auto &, const auto &) {
            return LocalLibraryScanResult{false, false, 2, QStringLiteral("enumeration failed")};
        }), repositoryFactory(state));
        coordinator.setLogger(&logger);
        QSignalSpy failed(&coordinator, &LocalLibraryScanCoordinator::failed);
        QVERIFY(coordinator.start(request()));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
        const auto entries = loggedScanLifecycle(logger, coordinator);
        QVERIFY(entries.contains(QStringLiteral("[ERROR] [LocalLibrary] Local library scan completed: outcome=failed, elapsedMs=")));
        QVERIFY(entries.contains(QStringLiteral("candidateFiles=2, error=enumeration failed")));
        QVERIFY(entries.contains(QStringLiteral("skippedFiles=0")));
    }
    {
        QFile::remove(scanLogPath());
        State state;
        AsyncLogger logger;
        logger.start();
        LocalLibraryScanCoordinator coordinator(scannerFactory(state, success()), [](QString &error) {
            error = QStringLiteral("repository unavailable");
            return std::unique_ptr<ILocalFileRepository>{};
        });
        coordinator.setLogger(&logger);
        QSignalSpy failed(&coordinator, &LocalLibraryScanCoordinator::failed);
        QVERIFY(coordinator.start(request()));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
        const auto entries = loggedScanLifecycle(logger, coordinator);
        QVERIFY(entries.contains(QStringLiteral("[ERROR] [LocalLibrary] Local library scan completed: outcome=failed, elapsedMs=")));
        QVERIFY(entries.contains(QStringLiteral("candidateFiles=0, error=repository unavailable")));
        QVERIFY(entries.contains(QStringLiteral("skippedFiles=0")));
    }
}

void LocalLibraryScanCoordinatorTests::logsInterruptedShutdown() {
    QStandardPaths::setTestModeEnabled(true);
    QFile::remove(scanLogPath());
    State state;
    QSemaphore entered;
    AsyncLogger logger;
    logger.start();
    LocalLibraryScanCoordinator coordinator(scannerFactory(state, [&](const auto &, const auto &, const auto &stop) {
        entered.release();
        while (!stop()) QThread::msleep(1);
        return LocalLibraryScanResult{false, true, 0, QStringLiteral("interrupted")};
    }), repositoryFactory(state));
    coordinator.setLogger(&logger);
    QVERIFY(coordinator.start(request()));
    QVERIFY(entered.tryAcquire(1, 3000));
    coordinator.shutdown();
    const auto entries = loggedScanLifecycle(logger, coordinator);
    QVERIFY(entries.contains(QStringLiteral("[WARN] [LocalLibrary] Local library scan completed: outcome=interrupted, elapsedMs=")));
    QVERIFY(entries.contains(QStringLiteral("candidateFiles=0, error=interrupted")));
    QVERIFY(entries.contains(QStringLiteral("skippedFiles=0")));
}

QTEST_GUILESS_MAIN(LocalLibraryScanCoordinatorTests)
#include "LocalLibraryScanCoordinatorTests.moc"
