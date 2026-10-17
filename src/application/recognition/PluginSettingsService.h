#ifndef HAIKENANIME_PLUGINSETTINGSSERVICE_H
#define HAIKENANIME_PLUGINSETTINGSSERVICE_H

#include <QHash>
#include <QList>
#include <QString>
#include <QVariantMap>

#include "IPluginSecretStore.h"
#include "IPluginSettingsRepository.h"
#include "PluginSettingsSchema.h"

/// Administers plugin schemas, validated drafts and persisted values. Secret values are write-only:
/// values() returns secretMask() for them and no error message ever contains them.
class PluginSettingsService final {
public:
    PluginSettingsService(IPluginSettingsRepository &repository, IPluginSecretStore &secrets);

    [[nodiscard]] static QString secretMask();

    /// Registers a plugin schema and loads its stored values. Rejects duplicate plugin ids.
    bool registerSchema(const PluginSettingsSchema &schema, QString &error);
    [[nodiscard]] QList<PluginSettingsSchema> schemas() const;
    /// Effective values (draft > stored > default) with secrets masked.
    [[nodiscard]] QVariantMap values(const QString &pluginId) const;
    [[nodiscard]] bool hasSecret(const QString &pluginId, const QString &key) const;

    bool setValue(const QString &pluginId, const QString &key, const QVariant &value, QString &error);
    /// Validates required fields, then persists non-secret values and pending secrets.
    bool save(const QString &pluginId, QString &error);
    bool clearSecret(const QString &pluginId, const QString &key, QString &error);

private:
    struct PluginState {
        PluginSettingsSchema schema;
        QVariantMap stored;
        QVariantMap draft;
        QHash<QString, QString> pendingSecrets;
    };

    IPluginSettingsRepository &repository_;
    IPluginSecretStore &secrets_;
    QList<QString> order_;
    QHash<QString, PluginState> plugins_;
};

#endif // HAIKENANIME_PLUGINSETTINGSSERVICE_H
