#include "application/cache/ClearLocalCacheUseCase.h"

#include <QSemaphore>
#include <QtTest>

#include <atomic>
#include <thread>

class CleanupFakeDownloader final : public ICoverDownloader {
public:
    quint64 Start(const CoverRequest &, Completion) override { return ++nextId; }
    void Cancel(quint64) override {}

    quint64 nextId = 0;
};

class CleanupFakeCache final : public ICoverCacheRepository {
public:
    bool ReadAll(QHash<int, CoverCacheEntry> &out, QString &) override
    {
        out = entries;
        return true;
    }
    bool Upsert(const CoverCacheEntry &entry, QString &) override
    {
        entries.insert(entry.mediaId, entry);
        return true;
    }
    bool Remove(int mediaId, QString &) override
    {
        entries.remove(mediaId);
        return true;
    }
    bool Clear(int &removedEntries, QString &error) override
    {
        if (failClear) {
            removedEntries = 0;
            error = QStringLiteral("cover metadata failure");
            return false;
        }
        removedEntries = entries.size();
        entries.clear();
        error.clear();
        return true;
    }

    QHash<int, CoverCacheEntry> entries;
    bool failClear = false;
};

class CleanupFakeFiles final : public ICoverFileStore {
public:
    bool Exists(const QString &path) const override { return files.contains(path); }
    bool Publish(int, const QString &, const QString &, const QString &,
                 PublishedCover &, QString &) override { return false; }
    bool Remove(const QString &path, QString &) override
    {
        files.remove(path);
        return true;
    }
    bool Clear(int &removedFiles, QString &error) override
    {
        if (failClear) {
            removedFiles = 0;
            error = QStringLiteral("cover file failure");
            return false;
        }
        removedFiles = files.size();
        files.clear();
        error.clear();
        return true;
    }
    bool RemoveOrphans(const QSet<QString> &, int, int &, QString &) override { return true; }
    QString AbsolutePath(const QString &path) const override { return path; }

    QSet<QString> files;
    bool failClear = false;
};

class CleanupFakeTemporaryStore final : public ICoverTemporaryStore {
public:
    bool ClearAbandoned(int &removedFiles, QString &error) const override
    {
        ++calls;
        removedFiles = removed;
        error = failure;
        return failure.isEmpty();
    }

    mutable int calls = 0;
    int removed = 0;
    QString failure;
};

class FakeCleanupParticipant final : public ICacheCleanupParticipant {
public:
    QString Name() const override { return name; }
    CacheCleanupParticipantResult Clear(const CacheCleanupCancellationProbe &isCancelled) override
    {
        ++calls;
        observedCancellation = isCancelled();
        if (onClear) onClear();
        return result;
    }

    QString name;
    CacheCleanupParticipantResult result;
    std::function<void()> onClear;
    int calls = 0;
    bool observedCancellation = false;
};

class BlockingCleanupParticipant final : public ICacheCleanupParticipant {
public:
    QString Name() const override { return QStringLiteral("blocking-derived-data"); }
    CacheCleanupParticipantResult Clear(const CacheCleanupCancellationProbe &) override
    {
        entered.release();
        resume.acquire();
        return {};
    }

    QSemaphore entered;
    QSemaphore resume;
};

class ClearLocalCacheUseCaseTests final : public QObject {
    Q_OBJECT

private slots:
    void clearsOnlyCoversAndRegisteredDerivedParticipants();
    void reportsPartialFailuresWithoutClaimingSuccess();
    void repeatedCleanupIsSafeAndReportsZeroAdditionalRemovals();
    void cancellationStopsBeforeTheNextParticipantAndReturnsToIdle();
    void acceptedExternalCancellationIsReportedByCompletion();
    void rejectsAConcurrentStart();
};

static CoverDownloadCoordinator MakeCoverCoordinator(CleanupFakeDownloader &downloader,
                                                      CleanupFakeCache &cache,
                                                      CleanupFakeFiles &files)
{
    CoverSettings settings;
    settings.maxRetries = 0;
    return CoverDownloadCoordinator(downloader, cache, files, settings);
}

void ClearLocalCacheUseCaseTests::clearsOnlyCoversAndRegisteredDerivedParticipants()
{
    CleanupFakeDownloader downloader;
    CleanupFakeCache cache;
    cache.entries.insert(1, CoverCacheEntry{.mediaId = 1});
    cache.entries.insert(2, CoverCacheEntry{.mediaId = 2});
    CleanupFakeFiles files;
    files.files = {QStringLiteral("1.png"), QStringLiteral("2.png")};
    auto covers = MakeCoverCoordinator(downloader, cache, files);
    CleanupFakeTemporaryStore temporaryFiles;
    temporaryFiles.removed = 5;
    FakeCleanupParticipant thumbnails;
    thumbnails.name = QStringLiteral("derived-thumbnails");
    thumbnails.result.removedItems = 3;
    FakeCleanupParticipant indexes;
    indexes.name = QStringLiteral("derived-indexes");
    indexes.result.removedItems = 4;
    FakeCleanupParticipant unregistered;
    unregistered.name = QStringLiteral("primary-library");
    unregistered.result.removedItems = 99;
    ClearLocalCacheUseCase useCase(covers, temporaryFiles, {&thumbnails, &indexes});

    ClearLocalCacheResult result;
    int completions = 0;
    QVERIFY(useCase.Start([&](ClearLocalCacheResult value) {
        result = std::move(value);
        ++completions;
    }));

    QCOMPARE(completions, 1);
    QVERIFY(result.Succeeded());
    QCOMPARE(result.removedCoverFiles, 2);
    QCOMPARE(result.removedCoverTemporaryFiles, 5);
    QCOMPARE(result.removedCoverEntries, 2);
    QCOMPARE(result.removedDerivedItems, 7);
    QVERIFY(result.failures.isEmpty());
    QCOMPARE(thumbnails.calls, 1);
    QCOMPARE(indexes.calls, 1);
    QCOMPARE(unregistered.calls, 0);
    QCOMPARE(temporaryFiles.calls, 1);
}

void ClearLocalCacheUseCaseTests::reportsPartialFailuresWithoutClaimingSuccess()
{
    CleanupFakeDownloader downloader;
    CleanupFakeCache cache;
    cache.failClear = true;
    CleanupFakeFiles files;
    files.files.insert(QStringLiteral("cover.png"));
    auto covers = MakeCoverCoordinator(downloader, cache, files);
    CleanupFakeTemporaryStore temporaryFiles;
    temporaryFiles.removed = 1;
    temporaryFiles.failure = QStringLiteral("temporary cleanup failure");
    FakeCleanupParticipant participant;
    participant.name = QStringLiteral("derived-search-index");
    participant.result = {2, QStringLiteral("index cleanup failure")};
    ClearLocalCacheUseCase useCase(covers, temporaryFiles, {&participant});

    ClearLocalCacheResult result;
    QVERIFY(useCase.Start([&](ClearLocalCacheResult value) { result = std::move(value); }));

    QVERIFY(!result.Succeeded());
    QCOMPARE(result.removedCoverFiles, 1);
    QCOMPARE(result.removedCoverTemporaryFiles, 1);
    QCOMPARE(result.removedCoverEntries, 0);
    QCOMPARE(result.removedDerivedItems, 2);
    QCOMPARE(result.failures.size(), 3);
    QCOMPARE(result.failures.at(0).participant, QStringLiteral("cover-metadata"));
    QCOMPARE(result.failures.at(1).participant, QStringLiteral("cover-temporary-files"));
    QCOMPARE(result.failures.at(2).participant, QStringLiteral("derived-search-index"));
}

void ClearLocalCacheUseCaseTests::repeatedCleanupIsSafeAndReportsZeroAdditionalRemovals()
{
    CleanupFakeDownloader downloader;
    CleanupFakeCache cache;
    cache.entries.insert(1, CoverCacheEntry{.mediaId = 1});
    CleanupFakeFiles files;
    files.files.insert(QStringLiteral("1.png"));
    auto covers = MakeCoverCoordinator(downloader, cache, files);
    CleanupFakeTemporaryStore temporaryFiles;
    temporaryFiles.removed = 1;
    FakeCleanupParticipant participant;
    participant.name = QStringLiteral("derived-data");
    participant.result.removedItems = 1;
    ClearLocalCacheUseCase useCase(covers, temporaryFiles, {&participant});

    ClearLocalCacheResult first;
    QVERIFY(useCase.Start([&](ClearLocalCacheResult value) { first = std::move(value); }));
    participant.result.removedItems = 0;
    temporaryFiles.removed = 0;
    ClearLocalCacheResult second;
    QVERIFY(useCase.Start([&](ClearLocalCacheResult value) { second = std::move(value); }));

    QVERIFY(first.Succeeded());
    QCOMPARE(first.TotalRemoved(), 4);
    QVERIFY(second.Succeeded());
    QCOMPARE(second.TotalRemoved(), 0);
    QCOMPARE(participant.calls, 2);
}

void ClearLocalCacheUseCaseTests::cancellationStopsBeforeTheNextParticipantAndReturnsToIdle()
{
    CleanupFakeDownloader downloader;
    CleanupFakeCache cache;
    CleanupFakeFiles files;
    auto covers = MakeCoverCoordinator(downloader, cache, files);
    CleanupFakeTemporaryStore temporaryFiles;
    FakeCleanupParticipant first;
    first.name = QStringLiteral("first-derived-data");
    first.result.removedItems = 1;
    FakeCleanupParticipant second;
    second.name = QStringLiteral("second-derived-data");
    ClearLocalCacheUseCase useCase(covers, temporaryFiles, {&first, &second});
    first.onClear = [&] { QVERIFY(useCase.Cancel()); };

    ClearLocalCacheResult result;
    ClearLocalCacheState completionState = ClearLocalCacheState::Running;
    QVERIFY(useCase.Start([&](ClearLocalCacheResult value) {
        result = std::move(value);
        completionState = useCase.State();
    }));

    QVERIFY(result.cancelled);
    QVERIFY(!result.Succeeded());
    QCOMPARE(first.calls, 1);
    QCOMPARE(second.calls, 0);
    QCOMPARE(completionState, ClearLocalCacheState::Idle);
    QCOMPARE(useCase.State(), ClearLocalCacheState::Idle);
}

void ClearLocalCacheUseCaseTests::acceptedExternalCancellationIsReportedByCompletion()
{
    CleanupFakeDownloader downloader;
    CleanupFakeCache cache;
    CleanupFakeFiles files;
    auto covers = MakeCoverCoordinator(downloader, cache, files);
    CleanupFakeTemporaryStore temporaryFiles;
    BlockingCleanupParticipant participant;
    ClearLocalCacheUseCase useCase(covers, temporaryFiles, {&participant});
    std::atomic_bool cancellationAccepted = false;
    std::thread canceller([&] {
        if (participant.entered.tryAcquire(1, 5000)) {
            cancellationAccepted.store(useCase.Cancel());
        }
        participant.resume.release();
    });

    ClearLocalCacheResult result;
    QVERIFY(useCase.Start([&](ClearLocalCacheResult value) { result = std::move(value); }));
    canceller.join();

    QVERIFY(cancellationAccepted.load());
    QVERIFY(result.cancelled);
    QVERIFY(!result.Succeeded());
    QCOMPARE(useCase.State(), ClearLocalCacheState::Idle);
}

void ClearLocalCacheUseCaseTests::rejectsAConcurrentStart()
{
    CleanupFakeDownloader downloader;
    CleanupFakeCache cache;
    CleanupFakeFiles files;
    auto covers = MakeCoverCoordinator(downloader, cache, files);
    CleanupFakeTemporaryStore temporaryFiles;
    FakeCleanupParticipant participant;
    participant.name = QStringLiteral("derived-data");
    ClearLocalCacheUseCase useCase(covers, temporaryFiles, {&participant});
    bool nestedStarted = true;
    participant.onClear = [&] { nestedStarted = useCase.Start({}); };

    QVERIFY(useCase.Start({}));

    QVERIFY(!nestedStarted);
    QCOMPARE(useCase.State(), ClearLocalCacheState::Idle);
}

QTEST_MAIN(ClearLocalCacheUseCaseTests)
#include "ClearLocalCacheUseCaseTests.moc"
