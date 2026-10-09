#include "WindowsCredentialStore.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <windows.h>
#include <wincred.h>

#include <utility>

namespace {
QString WinError(const DWORD code) {
    return QStringLiteral("Windows Credential Manager error %1.").arg(code);
}

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
    error.clear();
    PCREDENTIALW credential = nullptr;
    if (!CredReadW(reinterpret_cast<LPCWSTR>(targetName_.utf16()), CRED_TYPE_GENERIC, 0, &credential)) {
        error = WinError(GetLastError());
        return false;
    }

    const QByteArray payload(reinterpret_cast<const char *>(credential->CredentialBlob),
                             static_cast<qsizetype>(credential->CredentialBlobSize));
    CredFree(credential);
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
    const QByteArray payload = Serialize(credentials);
    CREDENTIALW credential{};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = reinterpret_cast<LPWSTR>(const_cast<ushort *>(targetName_.utf16()));
    credential.CredentialBlobSize = static_cast<DWORD>(payload.size());
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char *>(payload.constData()));
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
    credential.UserName = reinterpret_cast<LPWSTR>(const_cast<ushort *>(credentials.username.utf16()));
    if (!CredWriteW(&credential, 0)) {
        error = WinError(GetLastError());
        return false;
    }
    return true;
}

bool WindowsCredentialStore::clearAniListCredentials(QString &error) {
    error.clear();
    if (CredDeleteW(reinterpret_cast<LPCWSTR>(targetName_.utf16()), CRED_TYPE_GENERIC, 0)) {
        return true;
    }
    const DWORD code = GetLastError();
    if (code == ERROR_NOT_FOUND) {
        return true;
    }
    error = WinError(code);
    return false;
}
