#ifndef HAIKENANIME_WINDOWSPLUGINSECRETSTORE_H
#define HAIKENANIME_WINDOWSPLUGINSECRETSTORE_H

#include <QString>

#include "../../application/recognition/IPluginSecretStore.h"

/// Stores each plugin secret as one generic credential "<prefix>.<pluginId>.<key>" in
/// Windows Credential Manager. Error messages carry only Windows error codes.
class WindowsPluginSecretStore final : public IPluginSecretStore {
public:
    explicit WindowsPluginSecretStore(QString targetPrefix = QStringLiteral("HaikenAnime.Plugin"));

    [[nodiscard]] bool hasSecret(const QString &pluginId, const QString &key) const override;
    bool readSecret(const QString &pluginId, const QString &key, QString &value, QString &error) override;
    bool writeSecret(const QString &pluginId, const QString &key, const QString &value, QString &error) override;
    bool clearSecret(const QString &pluginId, const QString &key, QString &error) override;

private:
    [[nodiscard]] QString target(const QString &pluginId, const QString &key) const;

    QString targetPrefix_;
};

#endif // HAIKENANIME_WINDOWSPLUGINSECRETSTORE_H
