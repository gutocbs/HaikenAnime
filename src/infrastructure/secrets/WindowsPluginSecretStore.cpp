#include "WindowsPluginSecretStore.h"

#include <windows.h>
#include <wincred.h>

#include <utility>

namespace {
QString WinError(const DWORD code) {
    return QStringLiteral("Windows Credential Manager error %1.").arg(code);
}

LPCWSTR Wide(const QString &value) {
    return reinterpret_cast<LPCWSTR>(value.utf16());
}
}

WindowsPluginSecretStore::WindowsPluginSecretStore(QString targetPrefix) : targetPrefix_(std::move(targetPrefix)) {}

QString WindowsPluginSecretStore::target(const QString &pluginId, const QString &key) const {
    return targetPrefix_ + QLatin1Char('.') + pluginId + QLatin1Char('.') + key;
}

bool WindowsPluginSecretStore::hasSecret(const QString &pluginId, const QString &key) const {
    const auto name = target(pluginId, key);
    PCREDENTIALW credential = nullptr;
    if (!CredReadW(Wide(name), CRED_TYPE_GENERIC, 0, &credential)) return false;
    CredFree(credential);
    return true;
}

bool WindowsPluginSecretStore::readSecret(const QString &pluginId, const QString &key, QString &value, QString &error) {
    value.clear();
    error.clear();
    const auto name = target(pluginId, key);
    PCREDENTIALW credential = nullptr;
    if (!CredReadW(Wide(name), CRED_TYPE_GENERIC, 0, &credential)) {
        error = WinError(GetLastError());
        return false;
    }
    value = QString::fromUtf8(reinterpret_cast<const char *>(credential->CredentialBlob),
                              static_cast<qsizetype>(credential->CredentialBlobSize));
    CredFree(credential);
    return true;
}

bool WindowsPluginSecretStore::writeSecret(const QString &pluginId, const QString &key, const QString &value,
                                           QString &error) {
    error.clear();
    if (value.isEmpty()) {
        error = QStringLiteral("An empty secret cannot be stored.");
        return false;
    }
    auto name = target(pluginId, key);
    auto payload = value.toUtf8();
    CREDENTIALW credential{};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = reinterpret_cast<LPWSTR>(const_cast<ushort *>(name.utf16()));
    credential.CredentialBlobSize = static_cast<DWORD>(payload.size());
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(payload.data());
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
    const bool written = CredWriteW(&credential, 0);
    // Do not leave the plaintext copy in memory longer than needed.
    payload.fill('\0');
    if (!written) {
        error = WinError(GetLastError());
        return false;
    }
    return true;
}

bool WindowsPluginSecretStore::clearSecret(const QString &pluginId, const QString &key, QString &error) {
    error.clear();
    const auto name = target(pluginId, key);
    if (CredDeleteW(Wide(name), CRED_TYPE_GENERIC, 0)) return true;
    const DWORD code = GetLastError();
    if (code == ERROR_NOT_FOUND) return true;
    error = WinError(code);
    return false;
}
