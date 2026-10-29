#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QSignalSpy>
#include <QThread>
#include <QtTest>

#include <atomic>
#include <memory>

#include "../../src/app/LocalLibraryRecognitionCoordinator.h"

namespace {
struct SharedState final {
    std::atomic_int saveCalls = 0;
    std::atomic<qsizetype> savedRecords = 0;
    std::atomic_int saveDelayMs = 0;
    qsizetype pendingCount = 0;
    QList<Media> catalog;
    QList<LocalFileRecognitionRecord> folderRecords;
    std::atomic_int pendingReads = 0;
    std::atomic_int folderReads = 0;
    std::atomic_int aliveRepositories = 0;
    std::atomic_int maxAliveRepositories = 0;
    QMutex mutex;
    QStringList folderPaths;
    QList<LocalFileRecognitionRecord> saved;
};

class SlowFakeRepository final : public ILocalFileRecognitionRepository {
public:
    explicit SlowFakeRepository(SharedState &state) : state_(state) {
        const auto alive = ++state_.aliveRepositories;
        auto previous = state_.maxAliveRepositories.load();
        while (alive > previous && !state_.maxAliveRepositories.compare_exchange_weak(previous, alive)) {}
    }
    ~SlowFakeRepository() override { --state_.aliveRepositories; }

    bool readFilesUnderFolder(const QString &folderPath, QList<LocalFileRecognitionRecord> &records,
                              QString &) override {
        ++state_.folderReads;
        {
            QMutexLocker locker(&state_.mutex);
            state_.folderPaths.append(folderPath);
        }
        records = state_.folderRecords;
        return true;
    }
    bool readPendingRecognition(const QString &, QList<LocalFileRecognitionRecord> &pending, QString &) override {
        ++state_.pendingReads;
        pending.reserve(state_.pendingCount);
        for (qsizetype index = 0; index < state_.pendingCount; ++index) {
            LocalFileRecognitionRecord record;
            record.id = index + 1;
            record.fileName = QStringLiteral("[Group] Example - %1.mkv").arg(index % 90 + 1, 2, 10, QLatin1Char('0'));
            pending.append(record);
        }
        return true;
    }
    bool readRecognitionCatalog(QList<Media> &catalog, QString &) override {
        if (!state_.catalog.isEmpty()) {
            catalog = state_.catalog;
            return true;
        }
        Media media;
        media.Id = 42;
        media.Name = QStringLiteral("Example");
        catalog = {media};
        return true;
    }
    bool saveRecognitionBatch(const QList<LocalFileRecognitionRecord> &records, QString &) override {
        QThread::msleep(static_cast<unsigned long>(state_.saveDelayMs.load()));
        state_.savedRecords += records.size();
        {
            QMutexLocker locker(&state_.mutex);
            state_.saved.append(records);
        }
        ++state_.saveCalls;
        return true;
    }

private:
    SharedState &state_;
};
}

class LocalLibraryRecognitionCoordinatorTests final : public QObject {
    Q_OBJECT

private slots:
    void shutdownDuringALongRunReturnsQuicklyAndKeepsPersistedBatches();
    void completesAndPersistsEveryRecordWhenNotCancelled();
    void startForFolderReassociatesAssociatedFilesAndNeverReadsTheRest();
    void concurrentRequestsAreQueuedAndCoalescedIntoOneFollowUpRun();
    void shutdownDuringAFolderRunStopsQuickly();
};

void LocalLibraryRecognitionCoordinatorTests::shutdownDuringALongRunReturnsQuicklyAndKeepsPersistedBatches() {
    SharedState state;
    state.pendingCount = 300'000;
    state.saveDelayMs = 5;
    LocalLibraryRecognitionCoordinator coordinator(
        [&state](QString &) { return std::make_unique<SlowFakeRepository>(state); });
    QSignalSpy completed(&coordinator, &LocalLibraryRecognitionCoordinator::completed);
    QVERIFY(coordinator.startForPending(QStringLiteral("D:/Library")));
    QTRY_VERIFY_WITH_TIMEOUT(state.saveCalls.load() >= 1, 20000);

    QElapsedTimer elapsed;
    elapsed.start();
    coordinator.shutdown();

    QVERIFY2(elapsed.elapsed() < 3000, qPrintable(QStringLiteral("shutdown took %1 ms").arg(elapsed.elapsed())));
    QVERIFY(state.savedRecords.load() >= 200);
    QVERIFY(state.savedRecords.load() < state.pendingCount);
    QTest::qWait(50);
    QCOMPARE(completed.count(), 0);
}

void LocalLibraryRecognitionCoordinatorTests::completesAndPersistsEveryRecordWhenNotCancelled() {
    SharedState state;
    state.pendingCount = 450;
    LocalLibraryRecognitionCoordinator coordinator(
        [&state](QString &) { return std::make_unique<SlowFakeRepository>(state); });
    QSignalSpy completed(&coordinator, &LocalLibraryRecognitionCoordinator::completed);

    QVERIFY(coordinator.startForPending(QStringLiteral("D:/Library")));

    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 20000);
    QCOMPARE(completed.first().at(0).value<qsizetype>(), 450);
    QCOMPARE(completed.first().at(1).value<qsizetype>(), 450);
    QCOMPARE(state.savedRecords.load(), 450);
}

void LocalLibraryRecognitionCoordinatorTests::startForFolderReassociatesAssociatedFilesAndNeverReadsTheRest() {
    SharedState state;
    Media wrong;
    wrong.Id = 99;
    wrong.Name = QStringLiteral("Kusuriya no Hitorigoto");
    Media owner;
    owner.Id = 195516;
    owner.Name = QStringLiteral("Kusuriya no Hitorigoto 3rd Season");
    owner.LocalPath = QStringLiteral("Q:\\Animes\\Kusuriya no Hitorigoto 3rd Season");
    state.catalog = {wrong, owner};
    LocalFileRecognitionRecord record;
    record.id = 7;
    record.rootPath = QStringLiteral("Q:/");
    record.relativePath = QStringLiteral(
        "Animes/Kusuriya no Hitorigoto 3rd Season/[Erai-raws] Kusuriya no Hitorigoto 3rd Season - 01 [1080p].mkv");
    record.fileName = QStringLiteral("[Erai-raws] Kusuriya no Hitorigoto 3rd Season - 01 [1080p].mkv");
    record.recognitionState = QStringLiteral("associated");
    record.mediaId = 99;
    state.folderRecords = {record};
    LocalLibraryRecognitionCoordinator coordinator(
        [&state](QString &) { return std::make_unique<SlowFakeRepository>(state); });
    QSignalSpy completed(&coordinator, &LocalLibraryRecognitionCoordinator::completed);

    QVERIFY(coordinator.startForFolder(QStringLiteral("Q:\\Animes\\Kusuriya no Hitorigoto 3rd Season")));

    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 20000);
    QCOMPARE(state.pendingReads.load(), 0);
    QCOMPARE(state.folderReads.load(), 1);
    QCOMPARE(state.folderPaths, QStringList{QStringLiteral("Q:\\Animes\\Kusuriya no Hitorigoto 3rd Season")});
    QCOMPARE(state.saved.size(), 1);
    QCOMPARE(state.saved.first().id, 7);
    QCOMPARE(state.saved.first().recognitionState, QStringLiteral("associated"));
    QCOMPARE(state.saved.first().mediaId, std::optional<int>(195516));
    QCOMPARE(state.saved.first().episode, std::optional<int>(1));
    QVERIFY(!coordinator.startForFolder(QString()));
}

void LocalLibraryRecognitionCoordinatorTests::concurrentRequestsAreQueuedAndCoalescedIntoOneFollowUpRun() {
    SharedState state;
    state.pendingCount = 3;
    state.saveDelayMs = 150;
    LocalFileRecognitionRecord record;
    record.id = 1;
    record.fileName = QStringLiteral("[Group] Example - 01.mkv");
    state.folderRecords = {record};
    LocalLibraryRecognitionCoordinator coordinator(
        [&state](QString &) { return std::make_unique<SlowFakeRepository>(state); });
    QSignalSpy completed(&coordinator, &LocalLibraryRecognitionCoordinator::completed);

    QVERIFY(coordinator.startForPending(QStringLiteral("Q:/")));
    QVERIFY(coordinator.startForFolder(QStringLiteral("Q:/A")));
    QVERIFY(coordinator.startForFolder(QStringLiteral("Q:/A")));
    QVERIFY(coordinator.startForFolder(QStringLiteral("Q:/B")));
    QVERIFY(coordinator.startForPending(QStringLiteral("Q:/")));
    QVERIFY(coordinator.startForPending(QStringLiteral("Q:/")));

    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 2, 20000);
    QTest::qWait(300);
    QCOMPARE(completed.count(), 2);
    QCOMPARE(state.maxAliveRepositories.load(), 1);
    QCOMPARE(state.pendingReads.load(), 2);
    QCOMPARE(state.folderReads.load(), 2);
    QStringList folders = state.folderPaths;
    folders.sort();
    QCOMPARE(folders, (QStringList{QStringLiteral("Q:/A"), QStringLiteral("Q:/B")}));
}

void LocalLibraryRecognitionCoordinatorTests::shutdownDuringAFolderRunStopsQuickly() {
    SharedState state;
    state.saveDelayMs = 5;
    for (int index = 0; index < 100'000; ++index) {
        LocalFileRecognitionRecord record;
        record.id = index + 1;
        record.fileName = QStringLiteral("[Group] Example - %1.mkv").arg(index % 90 + 1, 2, 10, QLatin1Char('0'));
        state.folderRecords.append(record);
    }
    LocalLibraryRecognitionCoordinator coordinator(
        [&state](QString &) { return std::make_unique<SlowFakeRepository>(state); });
    QSignalSpy completed(&coordinator, &LocalLibraryRecognitionCoordinator::completed);
    QVERIFY(coordinator.startForFolder(QStringLiteral("Q:/Animes")));
    QTRY_VERIFY_WITH_TIMEOUT(state.saveCalls.load() >= 1, 20000);

    QElapsedTimer elapsed;
    elapsed.start();
    coordinator.shutdown();

    QVERIFY2(elapsed.elapsed() < 3000, qPrintable(QStringLiteral("shutdown took %1 ms").arg(elapsed.elapsed())));
    QVERIFY(state.savedRecords.load() < state.folderRecords.size());
    QTest::qWait(50);
    QCOMPARE(completed.count(), 0);
    QVERIFY(!coordinator.startForFolder(QStringLiteral("Q:/Animes")));
}

QTEST_GUILESS_MAIN(LocalLibraryRecognitionCoordinatorTests)
#include "LocalLibraryRecognitionCoordinatorTests.moc"
