#include "WindowsCredentialStore.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "WindowsCredentialManager.h"

#include <utility>

namespace {
bool IsValid(const AniListCredentials &credentials) {
    return !credentials.username.trimmed().isEmpty() && !credentials.token.trimmed().isEmpty()
        && !credentials.username.contains(QLatin1Char('\n'))
        && !credentials.username.contains(QLatin1Char('\r'))
        && !credentials.token.contains(QLatin1Char('\n'))
        && !credentials.token.contains(QLatin1Char('\r'));
}

QByteArray Serialize(const AniListCredentials &credentials) {
    QJsonObject object;
    object.insert(QStringLiteral("username"), credentials.username);
    object.insert(QStringLiteral("token"), credentials.token);
    object.insert(QStringLiteral("userId"), static_cast<double>(credentials.userId));
    object.insert(QStringLiteral("expiresAtUnixSeconds"), static_cast<double>(credentials.expiresAtUnixSeconds));
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

bool Deserialize(const QByteArray &payload, AniListCredentials &credentials) {
    const auto document = QJsonDocument::fromJson(payload);
    if (!document.isObject()) {
        return false;
    }
    const auto object = document.object();
    credentials.username = object.value(QStringLiteral("username")).toString();
    credentials.token = object.value(QStringLiteral("token")).toString();
    credentials.userId = static_cast<qint64>(object.value(QStringLiteral("userId")).toDouble());
    credentials.expiresAtUnixSeconds = static_cast<qint64>(
        object.value(QStringLiteral("expiresAtUnixSeconds")).toDouble());
    return IsValid(credentials);
}
}

WindowsCredentialStore::WindowsCredentialStore(QString targetName)
    : targetName_(std::move(targetName)) {
}

bool WindowsCredentialStore::loadAniListCredentials(AniListCredentials &credentials, QString &error) {
    credentials = {};
    QByteArray payload;
    if (!WindowsCredentialManager::read(targetName_, payload, error)) return false;
    if (!Deserialize(payload, credentials)) {
        credentials = {};
        error = QStringLiteral("Stored AniList credentials are invalid.");
        return false;
    }
    return true;
}

bool WindowsCredentialStore::saveAniListCredentials(const AniListCredentials &credentials, QString &error) {
    error.clear();
    if (!IsValid(credentials)) {
        error = QStringLiteral("AniList credentials must contain non-empty single-line username and token values.");
        return false;
    }
    return WindowsCredentialManager::write(targetName_, Serialize(credentials), credentials.username, error);
}

bool WindowsCredentialStore::clearAniListCredentials(QString &error) {
    return WindowsCredentialManager::remove(targetName_, error);
}
