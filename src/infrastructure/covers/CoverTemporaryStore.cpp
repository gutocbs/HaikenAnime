#include "CoverTemporaryStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#ifdef Q_OS_WIN
#include <windows.h>
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
    struct HandleCloser final {
        HANDLE handle;
        ~HandleCloser() { CloseHandle(handle); }
    } closer{rootHandle};
    FILE_ATTRIBUTE_TAG_INFO rootAttributes{};
    if (!GetFileInformationByHandleEx(rootHandle, FileAttributeTagInfo,
                                      &rootAttributes, sizeof(rootAttributes))
        || !(rootAttributes.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        || (rootAttributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
        error = QStringLiteral("The cover temporary root is not a safe directory.");
        return false;
    }
#else
    const QFileInfo rootInfo(rootPath_);
    if (!rootInfo.exists()) return true;
    if (!rootInfo.isDir() || rootInfo.isSymLink() || rootInfo.isJunction()) {
        error = QStringLiteral("The cover temporary root is not a safe directory.");
        return false;
    }
#endif

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
}
