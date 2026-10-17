#include <QElapsedTimer>
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
};

class SlowFakeRepository final : public ILocalFileRepository {
public:
    explicit SlowFakeRepository(SharedState &state) : state_(state) {}

    bool beginScan(const QString &, qint64 &, QString &) override { return false; }
    bool upsertBatch(qint64, const QList<LocalFileObservation> &, QString &) override { return false; }
    bool completeScan(qint64, qsizetype, QString &) override { return false; }
    bool failScan(qint64, LibraryScanStatus, qsizetype, const QString &, QString &) override { return false; }

    bool readPendingRecognition(const QString &, QList<LocalFileRecognitionRecord> &pending, QString &) override {
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
        Media media;
        media.Id = 42;
        media.Name = QStringLiteral("Example");
        catalog = {media};
        return true;
    }
    bool saveRecognitionBatch(const QList<LocalFileRecognitionRecord> &records, QString &) override {
        QThread::msleep(static_cast<unsigned long>(state_.saveDelayMs.load()));
        state_.savedRecords += records.size();
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
    void shutdownDuringALongRunReturnsQuicklyAndKeepsProcessedRecords();
    void completesAndPersistsEveryRecordWhenNotCancelled();
};

void LocalLibraryRecognitionCoordinatorTests::shutdownDuringALongRunReturnsQuicklyAndKeepsProcessedRecords() {
    SharedState state;
    state.pendingCount = 300'000;
    state.saveDelayMs = 5;
    LocalLibraryRecognitionCoordinator coordinator(
        [&state](QString &) { return std::make_unique<SlowFakeRepository>(state); });
    QVERIFY(coordinator.start(QStringLiteral("D:/Library")));
    QTRY_VERIFY_WITH_TIMEOUT(state.saveCalls.load() >= 1, 20000);

    QElapsedTimer elapsed;
    elapsed.start();
    coordinator.shutdown();

    QVERIFY2(elapsed.elapsed() < 1000, qPrintable(QStringLiteral("shutdown took %1 ms").arg(elapsed.elapsed())));
    QVERIFY(state.savedRecords.load() >= 200);
    QVERIFY(state.savedRecords.load() < state.pendingCount);
}

void LocalLibraryRecognitionCoordinatorTests::completesAndPersistsEveryRecordWhenNotCancelled() {
    SharedState state;
    state.pendingCount = 450;
    LocalLibraryRecognitionCoordinator coordinator(
        [&state](QString &) { return std::make_unique<SlowFakeRepository>(state); });
    QSignalSpy completed(&coordinator, &LocalLibraryRecognitionCoordinator::completed);

    QVERIFY(coordinator.start(QStringLiteral("D:/Library")));

    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 20000);
    QCOMPARE(completed.first().at(0).value<qsizetype>(), 450);
    QCOMPARE(completed.first().at(1).value<qsizetype>(), 450);
    QCOMPARE(state.savedRecords.load(), 450);
}

QTEST_GUILESS_MAIN(LocalLibraryRecognitionCoordinatorTests)
#include "LocalLibraryRecognitionCoordinatorTests.moc"
