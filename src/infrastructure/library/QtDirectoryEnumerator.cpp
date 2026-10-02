#include "QtDirectoryEnumerator.h"

#include <QDir>
#include <QDirIterator>
#ifdef Q_OS_WIN
#include <qt_windows.h>

namespace {
class WindowsDirectoryListing final : public IDirectoryListing {
public:
    explicit WindowsDirectoryListing(const QString &path) {
        const auto pattern = QDir::toNativeSeparators(QDir(path).filePath(QStringLiteral("*")));
        handle_ = FindFirstFileW(reinterpret_cast<LPCWSTR>(pattern.utf16()), &data_);
        if (handle_ == INVALID_HANDLE_VALUE) {
            terminalError_ = GetLastError();
            // A successfully validated empty directory may have no matching entries.
            if (terminalError_ == ERROR_FILE_NOT_FOUND) terminalError_ = ERROR_NO_MORE_FILES;
        }
    }
    ~WindowsDirectoryListing() override {
        if (handle_ != INVALID_HANDLE_VALUE) FindClose(handle_);
    }
    DirectoryListingEntry next() override {
        if (terminalError_) return {{}, terminalError_};
        if (first_) {
            first_ = false;
        } else if (!FindNextFileW(handle_, &data_)) {
            terminalError_ = GetLastError();
            return {{}, terminalError_};
        }
        return {QString::fromWCharArray(data_.cFileName), 0};
    }
private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW data_{};
    DWORD terminalError_ = 0;
    bool first_ = true;
};
}
#endif

QtDirectoryEnumerator::QtDirectoryEnumerator(ListingFactory listingFactory)
    : listingFactory_(std::move(listingFactory)) {}

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
    auto listing = listingFactory_ ? listingFactory_(path) : std::make_unique<WindowsDirectoryListing>(path);
    if (!listing) {
        error = QStringLiteral("Cannot create directory listing: %1").arg(path);
        return false;
    }
    for (;;) {
        const auto entry = listing->next();
        if (entry.errorCode == ERROR_NO_MORE_FILES) break;
        if (entry.errorCode != 0) {
            error = QStringLiteral("Cannot read directory: %1 (Windows error %2)").arg(path).arg(entry.errorCode);
            return false;
        }
        if (entry.fileName == QStringLiteral(".") || entry.fileName == QStringLiteral("..")) continue;
        // The same owned handle supplies every name and EOF/error result; no
        // separate access probe can hide a later FindNextFile failure.
        if (!visitor(QFileInfo(QDir(path).filePath(entry.fileName)))) return true;
    }
#else
    QDirIterator iterator(path, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
                          QDirIterator::NoIteratorFlags);
    while (iterator.hasNext()) {
        iterator.next();
        if (!visitor(iterator.fileInfo())) return true;
    }
#endif
    if (!QFileInfo(path).isDir()) {
        error = QStringLiteral("Directory disappeared during traversal: %1").arg(path);
        return false;
    }
    return true;
}
