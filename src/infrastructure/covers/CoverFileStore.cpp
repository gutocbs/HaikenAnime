#include "CoverFileStore.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeDatabase>
#include <QSaveFile>

namespace {
const QSet<QString> AllowedMimeTypes{"image/png", "image/jpeg", "image/webp", "image/gif"};

QString ExtensionForFormat(const QByteArray &format)
{
    if (format == "jpeg" || format == "jpg") return "jpg";
    if (format == "png") return "png";
    if (format == "webp") return "webp";
    if (format == "gif") return "gif";
    return {};
}
}

CoverFileStore::CoverFileStore(QString rootPath, CoverSettings settings)
    : rootPath_(QDir::cleanPath(std::move(rootPath))), settings_(settings)
{
}

bool CoverFileStore::IsSafeRelativePath(const QString &relativePath) const
{
    return !relativePath.isEmpty() && QDir::isRelativePath(relativePath)
           && !QDir::cleanPath(relativePath).startsWith("..")
           && !relativePath.contains('/') && !relativePath.contains('\\');
}

QString CoverFileStore::AbsolutePath(const QString &relativePath) const
{
    return IsSafeRelativePath(relativePath) ? QDir(rootPath_).filePath(relativePath) : QString{};
}

bool CoverFileStore::Exists(const QString &relativePath) const
{
    const auto path = AbsolutePath(relativePath);
    return !path.isEmpty() && QFileInfo::exists(path) && QFileInfo(path).isFile();
}

bool CoverFileStore::Publish(int mediaId, const QString &remoteUrl,
                             const QString &temporaryPath, const QString &responseMime,
                             PublishedCover &result, QString &error)
{
    result = {};
    error.clear();
    const QFileInfo input(temporaryPath);
    if (!input.exists() || !input.isFile() || input.size() <= 0 || input.size() > settings_.maxResponseBytes) {
        error = "Cover file size is invalid.";
        return false;
    }

    QImageReader reader(temporaryPath);
    reader.setDecideFormatFromContent(true);
    const QSize dimensions = reader.size();
    const QString extension = ExtensionForFormat(reader.format().toLower());
    const QString detectedMime = QMimeDatabase().mimeTypeForFile(temporaryPath, QMimeDatabase::MatchContent).name();
    if (!reader.canRead() || extension.isEmpty() || !AllowedMimeTypes.contains(detectedMime)
        || (!responseMime.isEmpty() && !AllowedMimeTypes.contains(responseMime.section(';', 0, 0).trimmed().toLower()))
        || !dimensions.isValid() || dimensions.width() < settings_.minDimension
        || dimensions.height() < settings_.minDimension || dimensions.width() > settings_.maxDimension
        || dimensions.height() > settings_.maxDimension || reader.read().isNull()) {
        error = "Cover image content is invalid or outside configured limits.";
        return false;
    }

    const auto hash = QCryptographicHash::hash(remoteUrl.toUtf8(), QCryptographicHash::Sha256).toHex().left(16);
    const QString relativePath = QString::number(mediaId) + "-" + QString::fromLatin1(hash) + "." + extension;
    if (!QDir().mkpath(rootPath_)) {
        error = "Could not create the cover cache directory.";
        return false;
    }
    QFile source(temporaryPath);
    QSaveFile destination(AbsolutePath(relativePath));
    if (!source.open(QIODevice::ReadOnly) || !destination.open(QIODevice::WriteOnly)
        || destination.write(source.readAll()) != input.size() || !destination.commit()) {
        destination.cancelWriting();
        error = "Could not publish the cover file atomically.";
        return false;
    }
    result = {relativePath, detectedMime, input.size()};
    return true;
}

bool CoverFileStore::Remove(const QString &relativePath, QString &error)
{
    error.clear();
    const auto path = AbsolutePath(relativePath);
    if (path.isEmpty()) {
        error = "Invalid cover path.";
        return false;
    }
    return !QFileInfo::exists(path) || QFile::remove(path) || (error = "Could not remove the cover file.", false);
}

bool CoverFileStore::Clear(QString &error)
{
    error.clear();
    QDir directory(rootPath_);
    if (!directory.exists()) return true;
    const auto files = directory.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (const auto &file : files) {
        if (!directory.remove(file)) {
            error = "Could not clear every cover file.";
            return false;
        }
    }
    return true;
}

bool CoverFileStore::RemoveOrphans(const QSet<QString> &referencedPaths, int maxFiles,
                                   int &removedFiles, QString &error)
{
    removedFiles = 0;
    error.clear();
    if (maxFiles <= 0) return true;
    QDirIterator iterator(rootPath_, QDir::Files | QDir::NoDotAndDotDot);
    const int inspectionLimit = maxFiles + referencedPaths.size();
    int inspected = 0;
    while (iterator.hasNext() && inspected++ < inspectionLimit && removedFiles < maxFiles) {
        iterator.next();
        const QString name = iterator.fileName();
        if (referencedPaths.contains(name)) continue;
        if (!QFile::remove(iterator.filePath())) {
            error = "Could not remove an orphan cover file.";
            return false;
        }
        ++removedFiles;
    }
    return true;
}
