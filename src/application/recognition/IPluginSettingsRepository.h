#ifndef HAIKENANIME_IPLUGINSETTINGSREPOSITORY_H
#define HAIKENANIME_IPLUGINSETTINGSREPOSITORY_H

#include <QString>
#include <QVariantMap>

/// Persists non-secret plugin setting values. Secrets go through IPluginSecretStore.
class IPluginSettingsRepository {
public:
    virtual ~IPluginSettingsRepository() = default;

    /// Replaces values with the stored settings of the plugin (empty when none) and clears error on success.
    virtual bool read(const QString &pluginId, QVariantMap &values, QString &error) = 0;
    /// Atomically replaces every stored value of the plugin; keys absent from values are removed.
    virtual bool replace(const QString &pluginId, const QVariantMap &values, QString &error) = 0;
};

#endif // HAIKENANIME_IPLUGINSETTINGSREPOSITORY_H
