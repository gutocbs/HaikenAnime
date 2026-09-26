#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSet>
#include <QTemporaryDir>
#include <QtTest>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <winioctl.h>
#endif

#include "../../src/infrastructure/library/LocalLibraryScanner.h"
#include "../../src/infrastructure/library/QtDirectoryEnumerator.h"

namespace {
#ifdef Q_OS_WIN
class NonLinkReparseFixture final {
public:
    ~NonLinkReparseFixture() {
        if (handle_ != INVALID_HANDLE_VALUE) {
            if (attached_) {
                DWORD returned = 0;
                DeviceIoControl(handle_, FSCTL_DELETE_REPARSE_POINT, &buffer_,
                                REPARSE_GUID_DATA_BUFFER_HEADER_SIZE, nullptr, 0, &returned, nullptr);
            }
            CloseHandle(handle_);
        }
    }
    bool attach(const QString &path, QString &error) {
        const auto native = QDir::toNativeSeparators(path);
        handle_ = CreateFileW(reinterpret_cast<LPCWSTR>(native.utf16()), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                              OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (handle_ == INVALID_HANDLE_VALUE) {
            error = QStringLiteral("CreateFileW error %1").arg(GetLastError());
            return false;
        }
        // An opaque, non-Microsoft, non-name-surrogate tag models provider metadata
        // without depending on a user's cloud account or installing a filter driver.
        buffer_.ReparseTag = 0x00000042;
        buffer_.ReparseGuid = {0x59a4b745, 0x7a59, 0x4899, {0xa6, 0x52, 0x91, 0x48, 0xd9, 0xa0, 0x11, 0xc0}};
        DWORD returned = 0;
        attached_ = DeviceIoControl(handle_, FSCTL_SET_REPARSE_POINT, &buffer_,
                                   REPARSE_GUID_DATA_BUFFER_HEADER_SIZE, nullptr, 0, &returned, nullptr);
        if (!attached_) error = QStringLiteral("FSCTL_SET_REPARSE_POINT error %1").arg(GetLastError());
        return attached_;
    }
private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    REPARSE_GUID_DATA_BUFFER buffer_{};
    bool attached_ = false;
};
#endif

void WriteFile(const QString &path, const QByteArray &contents = "same content") {
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) qFatal("Cannot create fixture directory");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size()) qFatal("Cannot create fixture file");
}

struct CapturedScan {
    QList<LocalFileObservation> observations;
    QList<qsizetype> batchSizes;
    QList<LocalLibraryScanProgress> progress;
    LocalLibraryScanResult result;
};

CapturedScan Scan(ILocalLibraryScanner &scanner, LocalLibraryScanRequest request,
                  const std::function<bool()> &stop = [] { return false; }) {
    CapturedScan captured;
    captured.result = scanner.scan(request, [&](const auto &batch, QString &) {
        captured.batchSizes.append(batch.size());
        captured.observations.append(batch);
        return true;
    }, [&](const auto &value) { captured.progress.append(value); }, stop);
    return captured;
}

class ControlledDirectoryEnumerator final : public IDirectoryEnumerator {
public:
    explicit ControlledDirectoryEnumerator(QString unreadable = {}) : unreadable_(std::move(unreadable)) {}
    bool enumerate(const QString &path, const std::function<bool(const QFileInfo &)> &visitor,
                   QString &error) override {
        if (path == unreadable_) {
            error = QStringLiteral("Access denied: %1").arg(path);
            return false;
        }
        for (const auto &entry : QDir(path).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                                         QDir::Name)) {
            if (!visitor(entry)) break;
        }
        return true;
    }
private:
    QString unreadable_;
};
}

class LocalLibraryScannerTests final : public QObject {
    Q_OBJECT
private slots:
    void recursesAndFiltersExtensions();
    void preservesOriginalPathsAndBasicMetadata();
    void caseFoldsUnicodeIdentityAndPreservesRequestedRoot();
    void retainsDistinctPathsWithEqualContent();
    void excludesHiddenDirectoriesAndTemporaryFiles();
    void includesHiddenRegularAllowedVideos();
    void excludesDirectoryLinksAndJunctions();
    void excludesWindowsSymbolicLinks();
    void includesWindowsNonLinkReparseVideo();
    void exactBatchBoundaries_data();
    void exactBatchBoundaries();
    void boundsVisitedEntriesEvenWhenFilesAreFiltered();
    void pausesOnlyBetweenNonFinalBatches();
    void cooperativelyStops();
    void stopBeforeTraversal();
    void stopAtLastEntryStillInterrupts();
    void unreadableChildRetainsEarlierObservations();
    void batchConsumerFailureStopsTraversal();
    void invalidRequests_data();
    void invalidRequests();
    void rejectsFileAndJunctionRoots();
    void excludesWindowsTemporaryAttribute();
};

void LocalLibraryScannerTests::recursesAndFiltersExtensions() {
    QTemporaryDir root;
    QVERIFY(root.isValid());
    WriteFile(root.filePath("Show/Season/Episode.MkV"));
    WriteFile(root.filePath("movie.MP4"));
    WriteFile(root.filePath("notes.txt"));
    WriteFile(root.filePath("no-extension"));
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    const auto captured = Scan(scanner, {root.path(), {".mkv", ".mp4"}, 200, 0});
    QVERIFY2(captured.result.complete, qPrintable(captured.result.diagnostic));
    QVERIFY(!captured.result.interrupted);
    QCOMPARE(captured.result.candidateFiles, 2);
    QCOMPARE(captured.observations.size(), 2);
    QCOMPARE(captured.progress.last().visitedEntries, 6);
    QCOMPARE(captured.progress.last().candidateFiles, 2);
    QSet<QString> extensions;
    for (const auto &observation : captured.observations) extensions.insert(observation.extension);
    QCOMPARE(extensions, (QSet<QString>{".mkv", ".mp4"}));
}

void LocalLibraryScannerTests::preservesOriginalPathsAndBasicMetadata() {
    QTemporaryDir root;
    const auto path = root.filePath(QStringLiteral("Show/Season/Episode.MKV"));
    WriteFile(path, "12345");
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadWrite));
    const auto modified = QDateTime::fromString("2026-09-20T12:34:56Z", Qt::ISODate);
    QVERIFY(file.setFileTime(modified, QFileDevice::FileModificationTime));
    file.close();
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    const auto captured = Scan(scanner, {root.path(), {".mkv"}, 200, 0});
    QVERIFY(captured.result.complete);
    QCOMPARE(captured.observations.size(), 1);
    const auto &observation = captured.observations.first();
    QCOMPARE(observation.rootPath, root.path());
    QCOMPARE(observation.relativePath, QStringLiteral("Show/Season/Episode.MKV"));
    QCOMPARE(observation.normalizedRelativePath, QStringLiteral("show/season/episode.mkv"));
    QCOMPARE(observation.fileName, QStringLiteral("Episode.MKV"));
    QCOMPARE(observation.extension, QStringLiteral(".mkv"));
    QCOMPARE(observation.sizeBytes, 5);
    QCOMPARE(observation.modifiedAt.toUTC(), modified);
}

void LocalLibraryScannerTests::retainsDistinctPathsWithEqualContent() {
    QTemporaryDir root;
    WriteFile(root.filePath("First/Episode.mkv"));
    WriteFile(root.filePath("Second/Episode.mkv"));
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    const auto captured = Scan(scanner, {root.path(), {".mkv"}, 200, 0});
    QVERIFY(captured.result.complete);
    QCOMPARE(captured.observations.size(), 2);
    QVERIFY(captured.observations[0].normalizedRelativePath != captured.observations[1].normalizedRelativePath);
}

void LocalLibraryScannerTests::caseFoldsUnicodeIdentityAndPreservesRequestedRoot() {
    QTemporaryDir root;
    const auto filename = QString::fromUtf8("Σς.MKV");
    WriteFile(root.filePath(filename));
    auto requestedRoot = QDir::toNativeSeparators(root.path());
    requestedRoot += QDir::separator();
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    const auto captured = Scan(scanner, {requestedRoot, {".mkv"}, 200, 0});
    QVERIFY(captured.result.complete);
    QCOMPARE(captured.observations.size(), 1);
    QCOMPARE(captured.observations.first().rootPath, requestedRoot);
    QCOMPARE(captured.observations.first().relativePath, filename);
    QCOMPARE(captured.observations.first().normalizedRelativePath, QString::fromUtf8("σσ.mkv"));
}

void LocalLibraryScannerTests::excludesHiddenDirectoriesAndTemporaryFiles() {
    QTemporaryDir root;
    WriteFile(root.filePath(".hidden/secret.mkv"));
    WriteFile(root.filePath("Visible/Episode.mkv"));
    WriteFile(root.filePath("Episode.mkv.part"));
    WriteFile(root.filePath("Episode.tmp"));
    WriteFile(root.filePath("~$Episode.mkv"));
#ifdef Q_OS_WIN
    WriteFile(root.filePath("HiddenAttribute/secret.mkv"));
    const auto hidden = QDir::toNativeSeparators(root.filePath("HiddenAttribute"));
    QVERIFY(SetFileAttributesW(reinterpret_cast<LPCWSTR>(hidden.utf16()), FILE_ATTRIBUTE_HIDDEN));
#endif
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    const auto captured = Scan(scanner, {root.path(), {".mkv", ".part", ".tmp"}, 200, 0});
    QVERIFY(captured.result.complete);
    QCOMPARE(captured.observations.size(), 1);
    QCOMPARE(captured.observations.first().relativePath, QStringLiteral("Visible/Episode.mkv"));
}

void LocalLibraryScannerTests::includesHiddenRegularAllowedVideos() {
    QTemporaryDir root;
    WriteFile(root.filePath(".hidden-video.mkv"));
    WriteFile(root.filePath(".hidden-directory/excluded.mkv"));
    QSet<QString> expected{QStringLiteral(".hidden-video.mkv")};
#ifdef Q_OS_WIN
    WriteFile(root.filePath("HiddenVideo.MKV"));
    const auto hidden = QDir::toNativeSeparators(root.filePath("HiddenVideo.MKV"));
    QVERIFY(SetFileAttributesW(reinterpret_cast<LPCWSTR>(hidden.utf16()), FILE_ATTRIBUTE_HIDDEN));
    expected.insert(QStringLiteral("HiddenVideo.MKV"));
#endif
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    const auto captured = Scan(scanner, {root.path(), {".mkv"}, 200, 0});
    QVERIFY2(captured.result.complete, qPrintable(captured.result.diagnostic));
    QSet<QString> actual;
    for (const auto &observation : captured.observations) actual.insert(observation.relativePath);
    QCOMPARE(actual, expected);
    QCOMPARE(captured.result.candidateFiles, qsizetype(expected.size()));
}

void LocalLibraryScannerTests::excludesDirectoryLinksAndJunctions() {
    QTemporaryDir root, external;
    WriteFile(root.filePath("ordinary.mkv"));
    WriteFile(external.filePath("outside.mkv"));
#ifdef Q_OS_WIN
    const auto junction = root.filePath("Junction");
    QProcess process;
    process.start("cmd.exe", {"/c", "mklink", "/J", QDir::toNativeSeparators(junction),
                               QDir::toNativeSeparators(external.path())});
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitCode(), 0);
    QVERIFY(QFileInfo(junction).isJunction());
#else
    QVERIFY(QFile::link(external.path(), root.filePath("Link")));
    QVERIFY(QFile::link(root.path(), root.filePath("Cycle")));
    QVERIFY(QFile::link(external.filePath("outside.mkv"), root.filePath("linked.mkv")));
#endif
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    const auto captured = Scan(scanner, {root.path(), {".mkv"}, 200, 0});
#ifdef Q_OS_WIN
    QVERIFY(QDir().rmdir(junction));
#endif
    QVERIFY(captured.result.complete);
    QCOMPARE(captured.observations.size(), 1);
    QCOMPARE(captured.observations.first().relativePath, QStringLiteral("ordinary.mkv"));
}

void LocalLibraryScannerTests::exactBatchBoundaries_data() {
    QTest::addColumn<int>("count");
    QTest::addColumn<QList<qsizetype>>("sizes");
    QTest::newRow("empty") << 0 << QList<qsizetype>{};
    QTest::newRow("one") << 1 << QList<qsizetype>{1};
    QTest::newRow("exact") << 200 << QList<qsizetype>{200};
    QTest::newRow("over") << 201 << QList<qsizetype>{200, 1};
    QTest::newRow("two-exact") << 400 << QList<qsizetype>{200, 200};
}

void LocalLibraryScannerTests::excludesWindowsSymbolicLinks() {
#ifdef Q_OS_WIN
    QTemporaryDir root, external;
    WriteFile(root.filePath("ordinary.mkv"));
    WriteFile(external.filePath("outside.mkv"));
    const auto linkPath = QDir::toNativeSeparators(root.filePath("SymbolicDirectory"));
    const auto targetPath = QDir::toNativeSeparators(external.path());
    if (!CreateSymbolicLinkW(reinterpret_cast<LPCWSTR>(linkPath.utf16()),
                             reinterpret_cast<LPCWSTR>(targetPath.utf16()),
                             SYMBOLIC_LINK_FLAG_DIRECTORY | 0x2)) {
        QSKIP("Windows has not enabled unprivileged symbolic links for this process");
    }
    const auto fileLink = QDir::toNativeSeparators(root.filePath("linked.mkv"));
    const auto fileTarget = QDir::toNativeSeparators(external.filePath("outside.mkv"));
    const bool fileCreated = CreateSymbolicLinkW(reinterpret_cast<LPCWSTR>(fileLink.utf16()),
                                                reinterpret_cast<LPCWSTR>(fileTarget.utf16()), 0x2);
    if (!fileCreated) {
        QDir().rmdir(QDir::fromNativeSeparators(linkPath));
        QFAIL("Could not create file symlink after creating directory symlink");
    }
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    const auto captured = Scan(scanner, {root.path(), {".mkv"}, 200, 0});
    QVERIFY(QFile::remove(QDir::fromNativeSeparators(fileLink)));
    QVERIFY(QDir().rmdir(QDir::fromNativeSeparators(linkPath)));
    QVERIFY(captured.result.complete);
    QCOMPARE(captured.observations.size(), 1);
    QCOMPARE(captured.observations.first().relativePath, QStringLiteral("ordinary.mkv"));
#else
    QSKIP("Native symbolic links are covered by excludesDirectoryLinksAndJunctions on this platform");
#endif
}

void LocalLibraryScannerTests::includesWindowsNonLinkReparseVideo() {
#ifdef Q_OS_WIN
    QTemporaryDir root;
    const auto path = root.filePath("provider-video.mkv");
    WriteFile(path, "metadata-only");
    NonLinkReparseFixture fixture;
    QString error;
    QVERIFY2(fixture.attach(path, error), qPrintable(error));
    const auto native = QDir::toNativeSeparators(path);
    QVERIFY(GetFileAttributesW(reinterpret_cast<LPCWSTR>(native.utf16())) & FILE_ATTRIBUTE_REPARSE_POINT);
    QVERIFY(!QFileInfo(path).isSymbolicLink());
    QVERIFY(!QFileInfo(path).isJunction());
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    const auto captured = Scan(scanner, {root.path(), {".mkv"}, 200, 0});
    QVERIFY2(captured.result.complete, qPrintable(captured.result.diagnostic));
    QCOMPARE(captured.result.candidateFiles, 1);
    QCOMPARE(captured.observations.size(), 1);
    QCOMPARE(captured.observations.first().relativePath, QStringLiteral("provider-video.mkv"));
    QCOMPARE(captured.observations.first().sizeBytes, 13);
#else
    QSKIP("Non-link Windows reparse tags are platform-specific");
#endif
}

void LocalLibraryScannerTests::exactBatchBoundaries() {
    QFETCH(int, count);
    QFETCH(QList<qsizetype>, sizes);
    QTemporaryDir root;
    for (int i = 0; i < count; ++i) WriteFile(root.filePath(QStringLiteral("%1.mkv").arg(i)));
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    LocalLibraryScanRequest request{root.path(), {".mkv"}};
    request.batchPauseMs = 0;
    const auto captured = Scan(scanner, request);
    QVERIFY(captured.result.complete);
    QCOMPARE(captured.batchSizes, sizes);
    QCOMPARE(captured.result.candidateFiles, count);
    QCOMPARE(captured.progress.last().visitedEntries, count);
}

void LocalLibraryScannerTests::boundsVisitedEntriesEvenWhenFilesAreFiltered() {
    QTemporaryDir root;
    for (int i = 0; i < 5; ++i) {
        WriteFile(root.filePath(QStringLiteral("%1.mkv").arg(i)));
        WriteFile(root.filePath(QStringLiteral("%1.txt").arg(i)));
    }
    ControlledDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    const auto captured = Scan(scanner, {root.path(), {".mkv"}, 2, 0});
    QVERIFY(captured.result.complete);
    QCOMPARE(captured.progress.last().visitedEntries, 10);
    QCOMPARE(captured.batchSizes, (QList<qsizetype>{1, 1, 1, 1, 1}));
}

void LocalLibraryScannerTests::pausesOnlyBetweenNonFinalBatches() {
    QTemporaryDir root;
    WriteFile(root.filePath("a.mkv"));
    WriteFile(root.filePath("b.mkv"));
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    QElapsedTimer timer;
    timer.start();
    const auto captured = Scan(scanner, {root.path(), {".mkv"}, 1, 100});
    QVERIFY(captured.result.complete);
    QVERIFY(timer.elapsed() >= 100);
    // A stop predicate must never run after final delivery: the scan is already finished.
    bool finalDelivered = false;
    bool checkedAfterFinal = false;
    QElapsedTimer sinceFinalDelivery;
    const auto result = scanner.scan({root.path(), {".mkv"}, 200, 1000},
        [&](const auto &, QString &) { finalDelivered = true; sinceFinalDelivery.start(); return true; }, {},
        [&] { checkedAfterFinal |= finalDelivered; return false; });
    QVERIFY(result.complete);
    QVERIFY(!checkedAfterFinal);
    QVERIFY(sinceFinalDelivery.elapsed() < 500);
}

void LocalLibraryScannerTests::cooperativelyStops() {
    QTemporaryDir root;
    for (int i = 0; i < 5; ++i) WriteFile(root.filePath(QStringLiteral("%1.mkv").arg(i)));
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    qsizetype visited = 0;
    QList<LocalFileObservation> observations;
    const auto result = scanner.scan({root.path(), {".mkv"}, 200, 0},
        [&](const auto &batch, QString &) { observations.append(batch); return true; },
        [&](const auto &value) { visited = value.visitedEntries; }, [&] { return visited >= 2; });
    QVERIFY(!result.complete);
    QVERIFY(result.interrupted);
    QCOMPARE(result.candidateFiles, 2);
    QCOMPARE(observations.size(), 2);
    QCOMPARE(visited, 2);
}

void LocalLibraryScannerTests::stopBeforeTraversal() {
    QTemporaryDir root;
    WriteFile(root.filePath("a.mkv"));
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    const auto captured = Scan(scanner, {root.path(), {".mkv"}, 200, 0}, [] { return true; });
    QVERIFY(captured.result.interrupted);
    QVERIFY(!captured.result.complete);
    QCOMPARE(captured.result.candidateFiles, 0);
    QVERIFY(captured.observations.isEmpty());
}

void LocalLibraryScannerTests::unreadableChildRetainsEarlierObservations() {
    QTemporaryDir root;
    WriteFile(root.filePath("a.mkv"));
    WriteFile(root.filePath("zUnreadable/missing.mkv"));
    ControlledDirectoryEnumerator enumerator(root.filePath("zUnreadable"));
    LocalLibraryScanner scanner(enumerator);
    const auto captured = Scan(scanner, {root.path(), {".mkv"}, 1, 0});
    QVERIFY(!captured.result.complete);
    QVERIFY(!captured.result.interrupted);
    QCOMPARE(captured.observations.size(), 1);
    QCOMPARE(captured.observations.first().relativePath, QStringLiteral("a.mkv"));
    QVERIFY(captured.result.diagnostic.contains(root.filePath("zUnreadable")));
}

void LocalLibraryScannerTests::stopAtLastEntryStillInterrupts() {
    QTemporaryDir root;
    WriteFile(root.filePath("one.mkv"));
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    bool stopped = false;
    int batches = 0;
    const auto result = scanner.scan({root.path(), {".mkv"}, 200, 0},
        [&](const auto &, QString &) { ++batches; return true; },
        [&](const auto &value) { stopped = value.visitedEntries > 0; }, [&] { return stopped; });
    QVERIFY(!result.complete);
    QVERIFY(result.interrupted);
    QCOMPARE(result.candidateFiles, 1);
    QCOMPARE(batches, 1);
}

void LocalLibraryScannerTests::batchConsumerFailureStopsTraversal() {
    QTemporaryDir root;
    for (int i = 0; i < 5; ++i) WriteFile(root.filePath(QStringLiteral("%1.mkv").arg(i)));
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    int calls = 0;
    const auto result = scanner.scan({root.path(), {".mkv"}, 1, 0},
        [&](const auto &, QString &error) { ++calls; error = "Repository unavailable"; return false; }, {}, {});
    QVERIFY(!result.complete);
    QVERIFY(!result.interrupted);
    QCOMPARE(calls, 1);
    QVERIFY(result.diagnostic.contains("Repository unavailable"));
}

void LocalLibraryScannerTests::invalidRequests_data() {
    QTest::addColumn<QString>("root");
    QTest::addColumn<qsizetype>("batchSize");
    QTest::addColumn<int>("pause");
    QTest::newRow("missing-root") << QStringLiteral("missing") << qsizetype(200) << 0;
    QTest::newRow("empty-root") << QString() << qsizetype(200) << 0;
    QTest::newRow("zero-batch") << QStringLiteral("valid") << qsizetype(0) << 0;
    QTest::newRow("oversized-batch") << QStringLiteral("valid") << qsizetype(201) << 0;
    QTest::newRow("negative-pause") << QStringLiteral("valid") << qsizetype(200) << -1;
}

void LocalLibraryScannerTests::invalidRequests() {
    QFETCH(QString, root);
    QFETCH(qsizetype, batchSize);
    QFETCH(int, pause);
    QTemporaryDir directory;
    if (root == "valid") root = directory.path();
    else if (root == "missing") root = directory.filePath("missing");
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    const auto captured = Scan(scanner, {root, {".mkv"}, batchSize, pause});
    QVERIFY(!captured.result.complete);
    QVERIFY(!captured.result.interrupted);
    QVERIFY(!captured.result.diagnostic.isEmpty());
    QVERIFY(captured.observations.isEmpty());
}

void LocalLibraryScannerTests::rejectsFileAndJunctionRoots() {
    QTemporaryDir root, target;
    WriteFile(root.filePath("file.mkv"));
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    auto captured = Scan(scanner, {root.filePath("file.mkv"), {".mkv"}, 200, 0});
    QVERIFY(!captured.result.complete);
    QVERIFY(captured.result.diagnostic.contains("file.mkv"));
#ifdef Q_OS_WIN
    const auto junction = root.filePath("Junction");
    QProcess process;
    process.start("cmd.exe", {"/c", "mklink", "/J", QDir::toNativeSeparators(junction),
                               QDir::toNativeSeparators(target.path())});
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitCode(), 0);
    captured = Scan(scanner, {junction, {".mkv"}, 200, 0});
    QVERIFY(QDir().rmdir(junction));
    QVERIFY(!captured.result.complete);
    QVERIFY(captured.observations.isEmpty());
#endif
}

void LocalLibraryScannerTests::excludesWindowsTemporaryAttribute() {
#ifdef Q_OS_WIN
    QTemporaryDir root;
    WriteFile(root.filePath("temporary.mkv"));
    const auto temporary = QDir::toNativeSeparators(root.filePath("temporary.mkv"));
    QVERIFY(SetFileAttributesW(reinterpret_cast<LPCWSTR>(temporary.utf16()), FILE_ATTRIBUTE_TEMPORARY));
    QtDirectoryEnumerator enumerator;
    LocalLibraryScanner scanner(enumerator);
    const auto captured = Scan(scanner, {root.path(), {".mkv"}, 200, 0});
    QVERIFY(captured.result.complete);
    QCOMPARE(captured.result.candidateFiles, 0);
    QVERIFY(captured.observations.isEmpty());
#else
    QSKIP("Windows temporary-file attributes are platform-specific");
#endif
}

QTEST_GUILESS_MAIN(LocalLibraryScannerTests)
#include "LocalLibraryScannerTests.moc"
