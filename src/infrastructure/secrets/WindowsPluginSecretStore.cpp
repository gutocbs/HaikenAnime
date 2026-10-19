#include "WindowsPluginSecretStore.h"

#include "WindowsCredentialManager.h"

#include <utility>

WindowsPluginSecretStore::WindowsPluginSecretStore(QString targetPrefix) : targetPrefix_(std::move(targetPrefix)) {}

QString WindowsPluginSecretStore::target(const QString &pluginId, const QString &key) const {
    return targetPrefix_ + QLatin1Char('.') + pluginId + QLatin1Char('.') + key;
}

bool WindowsPluginSecretStore::hasSecret(const QString &pluginId, const QString &key) const {
    return WindowsCredentialManager::exists(target(pluginId, key));
}

bool WindowsPluginSecretStore::readSecret(const QString &pluginId, const QString &key, QString &value, QString &error) {
    value.clear();
    QByteArray blob;
    if (!WindowsCredentialManager::read(target(pluginId, key), blob, error)) return false;
    value = QString::fromUtf8(blob);
    return true;
}

bool WindowsPluginSecretStore::writeSecret(const QString &pluginId, const QString &key, const QString &value,
                                           QString &error) {
    error.clear();
    if (value.isEmpty()) {
        error = QStringLiteral("An empty secret cannot be stored.");
        return false;
    }
    auto payload = value.toUtf8();
    const bool written = WindowsCredentialManager::write(target(pluginId, key), payload, {}, error);
    // Do not leave the plaintext copy in memory longer than needed.
    payload.fill('\0');
    return written;
}

bool WindowsPluginSecretStore::clearSecret(const QString &pluginId, const QString &key, QString &error) {
    return WindowsCredentialManager::remove(target(pluginId, key), error);
}
