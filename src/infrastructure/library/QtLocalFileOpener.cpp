#include "QtLocalFileOpener.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>

#include <utility>

QtLocalFileOpener::QtLocalFileOpener(OpenUrl openUrl) : openUrl_(std::move(openUrl)) {}

bool QtLocalFileOpener::open(const QString &path, QString &error) {
    error.clear();
    const QFileInfo file(path);
    if (path.trimmed().isEmpty() || !file.exists() || !file.isFile()) {
        error = QStringLiteral("Local media file does not exist: %1").arg(path);
        return false;
    }
    const auto url = QUrl::fromLocalFile(file.absoluteFilePath());
    const bool opened = openUrl_ ? openUrl_(url) : QDesktopServices::openUrl(url);
    if (!opened) {
        error = QStringLiteral("Could not open local media file: %1").arg(path);
        return false;
    }
    return true;
}
