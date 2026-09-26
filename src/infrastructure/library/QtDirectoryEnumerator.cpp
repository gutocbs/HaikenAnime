#include "QtDirectoryEnumerator.h"

#include <QDir>
#include <QDirIterator>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

bool QtDirectoryEnumerator::enumerate(const QString &path,
    const std::function<bool(const QFileInfo &)> &visitor, QString &error) {
    error.clear();
    const QFileInfo directory(path);
    if (!directory.exists() || !directory.isDir() || !directory.isReadable()
        || directory.isSymbolicLink() || directory.isJunction()) {
        error = QStringLiteral("Cannot read directory: %1").arg(path);
        return false;
    }
#ifdef Q_OS_WIN
    // Qt's iterator has no error result. Probe directory listing access so an ACL
    // denial cannot silently look like an empty directory and reconcile missing files.
    const auto pattern = QDir::toNativeSeparators(QDir(path).filePath(QStringLiteral("*")));
    WIN32_FIND_DATAW data;
    const HANDLE handle = FindFirstFileW(reinterpret_cast<LPCWSTR>(pattern.utf16()), &data);
    if (handle == INVALID_HANDLE_VALUE) {
        const auto code = GetLastError();
        if (code != ERROR_FILE_NOT_FOUND) {
            error = QStringLiteral("Cannot read directory: %1 (Windows error %2)").arg(path).arg(code);
            return false;
        }
    } else {
        FindClose(handle);
    }
#endif
    QDirIterator iterator(path, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
                          QDirIterator::NoIteratorFlags);
    while (iterator.hasNext()) {
        iterator.next();
        if (!visitor(iterator.fileInfo())) return true;
    }
    if (!QFileInfo(path).isDir()) {
        error = QStringLiteral("Directory disappeared during traversal: %1").arg(path);
        return false;
    }
    return true;
}
