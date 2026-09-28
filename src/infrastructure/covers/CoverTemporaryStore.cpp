#include "CoverTemporaryStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#include <winternl.h>

#include <array>
#endif

namespace {
#ifdef Q_OS_WIN
using NtCreateFileFunction = NTSTATUS(NTAPI *)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES,
    PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);

class NativeHandle final {
public:
    explicit NativeHandle(HANDLE handle = INVALID_HANDLE_VALUE) : handle_(handle) {}
    ~NativeHandle()
    {
        if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_);
    }
    NativeHandle(const NativeHandle &) = delete;
    NativeHandle &operator=(const NativeHandle &) = delete;
    HANDLE get() const { return handle_; }

private:
    HANDLE handle_;
};

bool isOwnedTemporaryName(const QString &name)
{
    return name.startsWith(QStringLiteral("cover-"), Qt::CaseInsensitive)
        && name.endsWith(QStringLiteral(".tmp"), Qt::CaseInsensitive);
}

bool removeRelativeFile(HANDLE rootHandle, const QString &fileName,
                        NtCreateFileFunction ntCreateFile)
{
    UNICODE_STRING relativeName{};
    relativeName.Length = static_cast<USHORT>(fileName.size() * sizeof(wchar_t));
    relativeName.MaximumLength = relativeName.Length;
    relativeName.Buffer = const_cast<PWSTR>(reinterpret_cast<LPCWSTR>(fileName.utf16()));

    OBJECT_ATTRIBUTES attributes{};
    attributes.Length = sizeof(attributes);
    attributes.RootDirectory = rootHandle;
    attributes.ObjectName = &relativeName;
    attributes.Attributes = OBJ_CASE_INSENSITIVE;

    HANDLE rawFileHandle = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK statusBlock{};
    const NTSTATUS openStatus = ntCreateFile(&rawFileHandle,
        DELETE | FILE_READ_ATTRIBUTES | SYNCHRONIZE, &attributes, &statusBlock, nullptr,
        FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_OPEN_REPARSE_POINT
            | FILE_SYNCHRONOUS_IO_NONALERT,
        nullptr, 0);
    if (openStatus < 0 || rawFileHandle == INVALID_HANDLE_VALUE) return false;
    const NativeHandle fileHandle(rawFileHandle);

    FILE_ATTRIBUTE_TAG_INFO fileAttributes{};
    if (!GetFileInformationByHandleEx(fileHandle.get(), FileAttributeTagInfo,
                                      &fileAttributes, sizeof(fileAttributes))
        || (fileAttributes.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        || (fileAttributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
        return false;
    }

    FILE_DISPOSITION_INFO disposition{};
    disposition.DeleteFile = TRUE;
    return SetFileInformationByHandle(fileHandle.get(), FileDispositionInfo,
                                      &disposition, sizeof(disposition));
}

bool clearAbandonedWindows(HANDLE rootHandle, int &removedFiles, QString &error)
{
    const HMODULE nativeLibrary = GetModuleHandleW(L"ntdll.dll");
    const auto ntCreateFile = nativeLibrary
        ? reinterpret_cast<NtCreateFileFunction>(GetProcAddress(nativeLibrary, "NtCreateFile"))
        : nullptr;
    if (!ntCreateFile) {
        error = QStringLiteral("The cover temporary root could not be cleaned safely.");
        return false;
    }

    bool complete = true;
    bool restart = true;
    alignas(FILE_ID_BOTH_DIR_INFO) std::array<unsigned char, 64 * 1024> buffer{};
    while (true) {
        const FILE_INFO_BY_HANDLE_CLASS infoClass = restart
            ? FileIdBothDirectoryRestartInfo
            : FileIdBothDirectoryInfo;
        restart = false;
        if (!GetFileInformationByHandleEx(rootHandle, infoClass,
                                          buffer.data(), static_cast<DWORD>(buffer.size()))) {
            if (GetLastError() == ERROR_NO_MORE_FILES) break;
            if (error.isEmpty()) {
                error = QStringLiteral("Could not enumerate abandoned cover downloads safely.");
            }
            return false;
        }

        auto *entry = reinterpret_cast<FILE_ID_BOTH_DIR_INFO *>(buffer.data());
        while (entry) {
            const QString fileName = QString::fromWCharArray(
                entry->FileName, static_cast<qsizetype>(entry->FileNameLength / sizeof(wchar_t)));
            if (!(entry->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                && !(entry->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                && isOwnedTemporaryName(fileName)) {
                if (removeRelativeFile(rootHandle, fileName, ntCreateFile)) {
                    ++removedFiles;
                } else {
                    complete = false;
                    if (error.isEmpty()) {
                        error = QStringLiteral("Could not remove an abandoned cover download: %1.")
                                    .arg(fileName);
                    }
                }
            }
            if (entry->NextEntryOffset == 0) break;
            entry = reinterpret_cast<FILE_ID_BOTH_DIR_INFO *>(
                reinterpret_cast<unsigned char *>(entry) + entry->NextEntryOffset);
        }
    }
    return complete;
}
#endif
}

#ifdef HAIKENANIME_COVER_TEMPORARY_STORE_TESTING
namespace {
std::function<void()> rootValidatedCallback;
}

void CoverTemporaryStoreTesting::SetRootValidatedCallback(std::function<void()> callback)
{
    rootValidatedCallback = std::move(callback);
}
#endif

CoverTemporaryStore::CoverTemporaryStore(QString rootPath)
    : rootPath_(QDir::cleanPath(std::move(rootPath)))
{
}

bool CoverTemporaryStore::ClearAbandoned(int &removedFiles, QString &error) const
{
    removedFiles = 0;
    error.clear();

#ifdef Q_OS_WIN
    // Do not share write/delete access: while this handle is held, no new handle
    // can mutate or replace the root path used by the direct-only Qt enumeration.
    const HANDLE rootHandle = CreateFileW(reinterpret_cast<LPCWSTR>(rootPath_.utf16()), FILE_LIST_DIRECTORY,
        FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (rootHandle == INVALID_HANDLE_VALUE) {
        const DWORD openError = GetLastError();
        if (openError == ERROR_FILE_NOT_FOUND || openError == ERROR_PATH_NOT_FOUND) return true;
        error = QStringLiteral("The cover temporary root could not be anchored safely.");
        return false;
    }
    const NativeHandle rootHandleCloser(rootHandle);
    FILE_ATTRIBUTE_TAG_INFO rootAttributes{};
    if (!GetFileInformationByHandleEx(rootHandle, FileAttributeTagInfo,
                                      &rootAttributes, sizeof(rootAttributes))
        || !(rootAttributes.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        || (rootAttributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
        error = QStringLiteral("The cover temporary root is not a safe directory.");
        return false;
    }
#ifdef HAIKENANIME_COVER_TEMPORARY_STORE_TESTING
    if (rootValidatedCallback) rootValidatedCallback();
#endif
    return clearAbandonedWindows(rootHandle, removedFiles, error);
#else
    const QFileInfo rootInfo(rootPath_);
    if (!rootInfo.exists()) return true;
    if (!rootInfo.isDir() || rootInfo.isSymLink() || rootInfo.isJunction()) {
        error = QStringLiteral("The cover temporary root is not a safe directory.");
        return false;
    }

    const QDir root(rootPath_);
    bool complete = true;
    const auto files = root.entryInfoList({QString::fromLatin1(CoverTemporaryFiles::NameFilter)},
        QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    for (const QFileInfo &file : files) {
        if (!QFile::remove(file.filePath())) {
            complete = false;
            if (error.isEmpty()) {
                error = QStringLiteral("Could not remove an abandoned cover download: %1.")
                            .arg(file.fileName());
            }
            continue;
        }
        ++removedFiles;
    }
    return complete;
#endif
}
