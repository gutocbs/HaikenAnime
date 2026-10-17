#ifndef HAIKENANIME_IPLUGINSECRETSTORE_H
#define HAIKENANIME_IPLUGINSECRETSTORE_H

#include <QString>

/// Protected storage for plugin secrets (tokens). Error messages must never contain secret values.
class IPluginSecretStore {
public:
    virtual ~IPluginSecretStore() = default;

    [[nodiscard]] virtual bool hasSecret(const QString &pluginId, const QString &key) const = 0;
    /// Intended for infrastructure adapters only; never forward the value to presentation or logs.
    virtual bool readSecret(const QString &pluginId, const QString &key, QString &value, QString &error) = 0;
    virtual bool writeSecret(const QString &pluginId, const QString &key, const QString &value, QString &error) = 0;
    virtual bool clearSecret(const QString &pluginId, const QString &key, QString &error) = 0;
};

#endif // HAIKENANIME_IPLUGINSECRETSTORE_H
