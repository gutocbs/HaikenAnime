#include "FileSecretStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>

#include <utility>

FileSecretStore::FileSecretStore(QString filePath)
    : filePath_(std::move(filePath)) {
    if (filePath_.isEmpty()) {
        const auto configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        filePath_ = QDir(configPath).filePath(QStringLiteral("anilist-secrets.txt"));
    }
}

bool FileSecretStore::loadAniListCredentials(AniListCredentials &credentials, QString &error) {
    credentials = {};
    error.clear();
    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("Could not open AniList secrets file: %1").arg(file.errorString());
        return false;
    }

    bool hasUsername = false;
    bool hasToken = false;
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const auto line = stream.readLine();
        const auto separator = line.indexOf(QLatin1Char('='));
        if (separator <= 0) {
            continue;
        }

        const auto key = line.left(separator).trimmed();
        const auto value = line.mid(separator + 1).trimmed();
        if (key == QStringLiteral("username")) {
            credentials.username = value;
            hasUsername = true;
        } else if (key == QStringLiteral("token")) {
            credentials.token = value;
            hasToken = true;
        }
    }

    if (!hasUsername || !hasToken) {
        error = QStringLiteral("AniList secrets file must contain username and token entries.");
        return false;
    }

    return true;
}

bool FileSecretStore::saveAniListCredentials(const AniListCredentials &credentials, QString &error) {
    error.clear();
    const QFileInfo fileInfo(filePath_);
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        error = QStringLiteral("Could not create the secrets directory.");
        return false;
    }

    QSaveFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        error = QStringLiteral("Could not open AniList secrets file for writing: %1")
                    .arg(file.errorString());
        return false;
    }

    QTextStream stream(&file);
    stream << "username=" << credentials.username << '\n';
    stream << "token=" << credentials.token << '\n';
    stream.flush();

    if (!file.commit()) {
        error = QStringLiteral("Could not commit AniList secrets file: %1").arg(file.errorString());
        return false;
    }

    return true;
}

QString FileSecretStore::filePath() const {
    return filePath_;
}
