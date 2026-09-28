#include "CoverTemporaryStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

CoverTemporaryStore::CoverTemporaryStore(QString rootPath)
    : rootPath_(QDir::cleanPath(std::move(rootPath)))
{
}

bool CoverTemporaryStore::ClearAbandoned(int &removedFiles, QString &error) const
{
    removedFiles = 0;
    error.clear();

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
}
