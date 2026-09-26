#include "LocalLibraryScanner.h"

#include <QDir>
#include <QSet>
#include <QThread>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace {
bool IsLink(const QFileInfo &entry) {
    if (entry.isSymbolicLink() || entry.isJunction()) return true;
#ifdef Q_OS_WIN
    const auto path = QDir::toNativeSeparators(entry.absoluteFilePath());
    const auto attributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(path.utf16()));
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT);
#else
    return false;
#endif
}

bool IsTemporary(const QFileInfo &entry) {
    const auto name = entry.fileName().toCaseFolded();
    if (name.startsWith(QStringLiteral("~$")) || name.endsWith(QLatin1Char('~'))) return true;
    const auto extension = entry.suffix().toCaseFolded();
    if (extension == QStringLiteral("tmp") || extension == QStringLiteral("temp")
        || extension == QStringLiteral("part") || extension == QStringLiteral("download")
        || extension == QStringLiteral("crdownload")) return true;
#ifdef Q_OS_WIN
    const auto path = QDir::toNativeSeparators(entry.absoluteFilePath());
    const auto attributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(path.utf16()));
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_TEMPORARY);
#else
    return false;
#endif
}
}

LocalLibraryScanner::LocalLibraryScanner(IDirectoryEnumerator &enumerator) : enumerator_(enumerator) {}

LocalLibraryScanResult LocalLibraryScanner::scan(const LocalLibraryScanRequest &request,
    const std::function<bool(const QList<LocalFileObservation> &, QString &)> &batchConsumer,
    const std::function<void(const LocalLibraryScanProgress &)> &progress,
    const std::function<bool()> &stopRequested) {
    LocalLibraryScanResult result;
    if (request.rootPath.trimmed().isEmpty() || request.allowedExtensions.isEmpty()
        || request.batchSize <= 0 || request.batchSize > 200 || request.batchPauseMs < 0 || !batchConsumer) {
        result.diagnostic = QStringLiteral("Invalid local library scan request.");
        return result;
    }

    const QDir root(request.rootPath);
    const QSet<QString> extensions(request.allowedExtensions.cbegin(), request.allowedExtensions.cend());
    LocalLibraryScanProgress current;
    QList<LocalFileObservation> batch;
    qsizetype entriesInBatch = 0;
    bool consumerFailed = false;

    const auto stopped = [&] {
        if (stopRequested && stopRequested()) {
            result.interrupted = true;
            return true;
        }
        return false;
    };
    const auto flush = [&] {
        if (batch.isEmpty()) return true;
        QString error;
        if (!batchConsumer(batch, error)) {
            result.diagnostic = error.isEmpty() ? QStringLiteral("Local library batch consumer failed.") : error;
            consumerFailed = true;
            return false;
        }
        batch.clear();
        return true;
    };

    std::function<bool(const QString &)> visitDirectory;
    visitDirectory = [&](const QString &path) {
        if (stopped()) return false;
        QString error;
        bool traversalSucceeded = true;
        const bool readable = enumerator_.enumerate(path, [&](const QFileInfo &entry) {
            if (stopped()) return traversalSucceeded = false;
            if (entriesInBatch == request.batchSize) {
                if (!flush()) return traversalSucceeded = false;
                entriesInBatch = 0;
                if (stopped()) return traversalSucceeded = false;
                if (request.batchPauseMs > 0) QThread::msleep(request.batchPauseMs);
                if (stopped()) return traversalSucceeded = false;
            }
            ++current.visitedEntries;
            ++entriesInBatch;
            const auto name = entry.fileName();
            const bool excluded = IsLink(entry) || entry.isHidden() || name.startsWith(QLatin1Char('.'));
            if (!excluded && entry.isFile() && !IsTemporary(entry)) {
                const auto extension = QLatin1Char('.') + entry.suffix().toLower();
                if (extensions.contains(extension)) {
                    const auto relative = QDir::fromNativeSeparators(root.relativeFilePath(entry.absoluteFilePath()));
                    batch.append({request.rootPath, relative, relative.toCaseFolded(), name, extension,
                                  entry.size(), entry.lastModified().toUTC()});
                    ++current.candidateFiles;
                }
            }
            if (progress) progress(current);
            if (!excluded && entry.isDir() && !visitDirectory(entry.absoluteFilePath())) {
                return traversalSucceeded = false;
            }
            return true;
        }, error);
        if (!readable) {
            result.diagnostic = error.isEmpty() ? QStringLiteral("Cannot read directory: %1").arg(path)
                                               : QStringLiteral("%1: %2").arg(path, error);
        }
        return readable && traversalSucceeded;
    };

    const bool traversed = visitDirectory(root.absolutePath());
    if (traversed && !consumerFailed) stopped();
    if (!consumerFailed) flush();
    result.candidateFiles = current.candidateFiles;
    result.complete = traversed && !consumerFailed && !result.interrupted;
    if (result.interrupted && result.diagnostic.isEmpty()) result.diagnostic = QStringLiteral("Local library scan interrupted.");
    if (progress) progress(current);
    return result;
}
