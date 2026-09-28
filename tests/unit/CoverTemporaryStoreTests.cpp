#include "infrastructure/covers/CoverTemporaryStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
void writeFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write("fixture") != 7) qFatal("Cannot write fixture");
}

class DeleteLockedFile final {
public:
    explicit DeleteLockedFile(const QString &path)
    {
#ifdef Q_OS_WIN
        handle_ = CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()), GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
#else
        Q_UNUSED(path)
#endif
    }
    ~DeleteLockedFile()
    {
#ifdef Q_OS_WIN
        if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_);
#endif
    }
    bool valid() const
    {
#ifdef Q_OS_WIN
        return handle_ != INVALID_HANDLE_VALUE;
#else
        return false;
#endif
    }
private:
#ifdef Q_OS_WIN
    HANDLE handle_ = INVALID_HANDLE_VALUE;
#endif
};
}

class CoverTemporaryStoreTests final : public QObject {
    Q_OBJECT
private slots:
    void missingRootIsAlreadyClean();
    void removesOnlyOwnedTemporaryFiles();
    void doesNotTraverseNestedDirectoriesOrLinks();
    void rejectsJunctionRootWithoutTouchingTarget();
    void declinesCleanupWhenRootCannotBeAnchored();
    void reportsPartialFailureForLockedOwnedFile();
};

void CoverTemporaryStoreTests::missingRootIsAlreadyClean()
{
    QTemporaryDir sandbox;
    const QString root = sandbox.filePath("does-not-exist");
    CoverTemporaryStore store(root);
    int removed = -1;
    QString error;

    QVERIFY2(store.ClearAbandoned(removed, error), qPrintable(error));
    QCOMPARE(removed, 0);
    QVERIFY(error.isEmpty());
    QVERIFY(!QFileInfo::exists(root));
}

void CoverTemporaryStoreTests::removesOnlyOwnedTemporaryFiles()
{
    QTemporaryDir sandbox;
    const QString root = sandbox.filePath("temporary-covers");
    const QString cache = sandbox.filePath("persistent-covers");
    QVERIFY(QDir().mkpath(root));
    QVERIFY(QDir().mkpath(cache));
    writeFile(root + "/cover-abandoned.tmp");
    writeFile(root + "/ordinary.txt");
    writeFile(cache + "/cover-persistent.tmp");
    CoverTemporaryStore store(root);
    int removed = 0;
    QString error;

    QVERIFY2(store.ClearAbandoned(removed, error), qPrintable(error));
    QCOMPARE(removed, 1);
    QVERIFY(!QFileInfo::exists(root + "/cover-abandoned.tmp"));
    QVERIFY(QFileInfo::exists(root + "/ordinary.txt"));
    QVERIFY(QFileInfo::exists(cache + "/cover-persistent.tmp"));
}

void CoverTemporaryStoreTests::doesNotTraverseNestedDirectoriesOrLinks()
{
    QTemporaryDir sandbox;
    const QString root = sandbox.filePath("temporary-covers");
    const QString nested = root + "/unexpected";
    const QString outside = sandbox.filePath("outside");
    QVERIFY(QDir().mkpath(nested));
    QVERIFY(QDir().mkpath(outside));
    writeFile(root + "/cover-root.tmp");
    writeFile(nested + "/cover-nested.tmp");
    writeFile(outside + "/cover-outside.tmp");
    const QString link = root + "/linked-outside";
    QVERIFY(QFile::link(outside, link));
    CoverTemporaryStore store(root);
    int removed = 0;
    QString error;

    QVERIFY2(store.ClearAbandoned(removed, error), qPrintable(error));
    QCOMPARE(removed, 1);
    QVERIFY(!QFileInfo::exists(root + "/cover-root.tmp"));
    QVERIFY(QFileInfo::exists(nested + "/cover-nested.tmp"));
    QVERIFY(QFileInfo::exists(outside + "/cover-outside.tmp"));
}

void CoverTemporaryStoreTests::rejectsJunctionRootWithoutTouchingTarget()
{
#ifndef Q_OS_WIN
    QSKIP("Windows junctions are not available on this platform.");
#else
    QTemporaryDir sandbox;
    const QString target = sandbox.filePath("outside");
    const QString junction = sandbox.filePath("temporary-covers");
    QVERIFY(QDir().mkpath(target));
    const QString outsideFile = QDir(target).filePath("cover-outside.tmp");
    writeFile(outsideFile);

    QProcess createJunction;
    const QString command = QStringLiteral("mklink /J \"%1\" \"%2\"")
                                .arg(QDir::toNativeSeparators(junction), QDir::toNativeSeparators(target));
    const QString commandProcessor = qEnvironmentVariable("COMSPEC");
    if (commandProcessor.isEmpty()) QSKIP("The Windows command processor is unavailable.");
    createJunction.setProgram(commandProcessor);
    createJunction.setNativeArguments(QStringLiteral("/C %1").arg(command));
    createJunction.start();
    if (!createJunction.waitForFinished() || createJunction.exitCode() != 0) {
        QSKIP(qPrintable(QStringLiteral("The test environment cannot create a Windows junction: %1")
            .arg(QString::fromLocal8Bit(createJunction.readAllStandardOutput()
                                        + createJunction.readAllStandardError()))));
    }
    QVERIFY(QFileInfo(junction).isJunction());

    CoverTemporaryStore store(junction);
    int removed = -1;
    QString error;

    QVERIFY(!store.ClearAbandoned(removed, error));
    QCOMPARE(removed, 0);
    QVERIFY(!error.isEmpty());
    QVERIFY(QFileInfo::exists(outsideFile));
    QVERIFY(QDir().rmdir(junction));
#endif
}

void CoverTemporaryStoreTests::declinesCleanupWhenRootCannotBeAnchored()
{
#ifndef Q_OS_WIN
    QSKIP("Windows directory-handle anchoring is not available on this platform.");
#else
    QTemporaryDir sandbox;
    const QString root = sandbox.filePath("temporary-covers");
    QVERIFY(QDir().mkpath(root));
    const QString ownedFile = QDir(root).filePath("cover-anchored.tmp");
    writeFile(ownedFile);
    const HANDLE rootHandle = CreateFileW(reinterpret_cast<LPCWSTR>(root.utf16()), GENERIC_READ,
        0, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    QVERIFY(rootHandle != INVALID_HANDLE_VALUE);

    CoverTemporaryStore store(root);
    int removed = -1;
    QString error;

    QVERIFY(!store.ClearAbandoned(removed, error));
    QCOMPARE(removed, 0);
    QVERIFY(!error.isEmpty());
    QVERIFY(QFileInfo::exists(ownedFile));
    CloseHandle(rootHandle);
#endif
}

void CoverTemporaryStoreTests::reportsPartialFailureForLockedOwnedFile()
{
#ifndef Q_OS_WIN
    QSKIP("This check uses Windows delete-sharing semantics.");
#else
    QTemporaryDir sandbox;
    const QString root = sandbox.filePath("temporary-covers");
    QVERIFY(QDir().mkpath(root));
    const QString removable = root + "/cover-removable.tmp";
    const QString locked = root + "/cover-locked.tmp";
    writeFile(removable);
    writeFile(locked);
    DeleteLockedFile lock(locked);
    QVERIFY(lock.valid());
    CoverTemporaryStore store(root);
    int removed = 0;
    QString error;

    QVERIFY(!store.ClearAbandoned(removed, error));
    QCOMPARE(removed, 1);
    QVERIFY(!error.isEmpty());
    QVERIFY(!QFileInfo::exists(removable));
    QVERIFY(QFileInfo::exists(locked));
#endif
}

QTEST_GUILESS_MAIN(CoverTemporaryStoreTests)
#include "CoverTemporaryStoreTests.moc"
