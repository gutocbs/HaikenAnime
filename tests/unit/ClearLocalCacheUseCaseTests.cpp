#include "application/cache/ClearLocalCacheUseCase.h"

#include <QtTest>

#include <functional>

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
    QString AbsolutePath(const QString &path) const override { return path; }

    QSet<QString> files;
    bool failClear = false;
};

class CleanupFakeTemporaryStore final : public ICoverTemporaryStore {
public:
    bool ClearAbandoned(int &removedFiles, QString &error) const override
    {
        ++calls;
        if (onClear) onClear();
        removedFiles = removed;
        error = failure;
        return failure.isEmpty();
    }

    mutable int calls = 0;
    int removed = 0;
    QString failure;
    std::function<void()> onClear;
};

class ClearLocalCacheUseCaseTests final : public QObject {
    Q_OBJECT

private slots:
    void clearsCoversAndTemporaryFiles();
    void reportsPartialFailuresWithoutClaimingSuccess();
    void repeatedCleanupIsSafeAndReportsZeroAdditionalRemovals();
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

void ClearLocalCacheUseCaseTests::clearsCoversAndTemporaryFiles()
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
    ClearLocalCacheUseCase useCase(covers, temporaryFiles);

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
    QVERIFY(result.failures.isEmpty());
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
    ClearLocalCacheUseCase useCase(covers, temporaryFiles);

    ClearLocalCacheResult result;
    QVERIFY(useCase.Start([&](ClearLocalCacheResult value) { result = std::move(value); }));

    QVERIFY(!result.Succeeded());
    QCOMPARE(result.removedCoverFiles, 1);
    QCOMPARE(result.removedCoverTemporaryFiles, 1);
    QCOMPARE(result.removedCoverEntries, 0);
    QCOMPARE(result.failures.size(), 2);
    QCOMPARE(result.failures.at(0).component, QStringLiteral("cover-metadata"));
    QCOMPARE(result.failures.at(1).component, QStringLiteral("cover-temporary-files"));
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
    ClearLocalCacheUseCase useCase(covers, temporaryFiles);

    ClearLocalCacheResult first;
    QVERIFY(useCase.Start([&](ClearLocalCacheResult value) { first = std::move(value); }));
    temporaryFiles.removed = 0;
    ClearLocalCacheResult second;
    QVERIFY(useCase.Start([&](ClearLocalCacheResult value) { second = std::move(value); }));

    QVERIFY(first.Succeeded());
    QCOMPARE(first.TotalRemoved(), 3);
    QVERIFY(second.Succeeded());
    QCOMPARE(second.TotalRemoved(), 0);
    QCOMPARE(temporaryFiles.calls, 2);
}

void ClearLocalCacheUseCaseTests::rejectsAConcurrentStart()
{
    CleanupFakeDownloader downloader;
    CleanupFakeCache cache;
    CleanupFakeFiles files;
    auto covers = MakeCoverCoordinator(downloader, cache, files);
    CleanupFakeTemporaryStore temporaryFiles;
    ClearLocalCacheUseCase useCase(covers, temporaryFiles);
    bool nestedStarted = true;
    temporaryFiles.onClear = [&] { nestedStarted = useCase.Start({}); };

    QVERIFY(useCase.Start({}));

    QVERIFY(!nestedStarted);
    temporaryFiles.onClear = {};
    QVERIFY(useCase.Start({}));
}

QTEST_MAIN(ClearLocalCacheUseCaseTests)
#include "ClearLocalCacheUseCaseTests.moc"
