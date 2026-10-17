#ifndef HAIKENANIME_PLUGINSETTINGSCONTROLLER_H
#define HAIKENANIME_PLUGINSETTINGSCONTROLLER_H

#include <QObject>
#include <QString>
#include <QVariantList>

#include <functional>

#include "../../application/recognition/PluginRegistry.h"
#include "../../application/recognition/PluginSettingsService.h"
#include "../../infrastructure/recognition/plex/PlexPlugin.h"

struct PlexAccountServices final {
    PlexAuthentication *authentication = nullptr;
    PlexApiClient *api = nullptr;
    PlexPlugin *plugin = nullptr;
};

/// Generic plugin settings screen: renders schema fields, keeps secrets write-only and exposes
/// capability actions (browser login, connection test, server and user selection).
class PluginSettingsController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList plugins READ plugins NOTIFY changed)
    Q_PROPERTY(QString selectedPluginId READ selectedPluginId NOTIFY changed)
    Q_PROPERTY(QVariantList fields READ fields NOTIFY changed)
    Q_PROPERTY(bool dirty READ dirty NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY changed)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY changed)
    Q_PROPERTY(QString connectionState READ connectionState NOTIFY changed)
    Q_PROPERTY(QVariantList servers READ servers NOTIFY changed)
    Q_PROPERTY(QVariantList users READ users NOTIFY changed)

public:
    PluginSettingsController(PluginSettingsService &settings, PluginRegistry &registry, PlexAccountServices plex = {},
                             std::function<void()> settingsApplied = {}, QObject *parent = nullptr);

    [[nodiscard]] QVariantList plugins() const;
    [[nodiscard]] QString selectedPluginId() const { return selectedPluginId_; }
    [[nodiscard]] QVariantList fields() const;
    [[nodiscard]] bool dirty() const;
    [[nodiscard]] bool busy() const { return busy_; }
    [[nodiscard]] QString errorMessage() const { return errorMessage_; }
    [[nodiscard]] QString statusMessage() const { return statusMessage_; }
    /// "disconnected", "connecting" or "connected" for plugins with browser authentication.
    [[nodiscard]] QString connectionState() const;
    [[nodiscard]] QVariantList servers() const { return servers_; }
    [[nodiscard]] QVariantList users() const;

    Q_INVOKABLE void SelectPlugin(const QString &pluginId);
    Q_INVOKABLE bool SetValue(const QString &key, const QVariant &value);
    Q_INVOKABLE bool ClearSecret(const QString &key);
    Q_INVOKABLE bool Save();
    Q_INVOKABLE void Discard();
    Q_INVOKABLE void Connect();
    Q_INVOKABLE void Disconnect();
    Q_INVOKABLE void TestConnection();
    Q_INVOKABLE void LoadServers();
    Q_INVOKABLE void SelectServer(const QString &serverId);
    Q_INVOKABLE void LoadUsers();
    Q_INVOKABLE void SetUserSelected(const QString &userId, bool selected);

signals:
    void changed();

private:
    void setMessages(const QString &error, const QString &status);
    [[nodiscard]] bool isPlexSelected() const;
    [[nodiscard]] QStringList selectedUserIds() const;

    PluginSettingsService &settings_;
    PluginRegistry &registry_;
    PlexAccountServices plex_;
    std::function<void()> settingsApplied_;
    QString selectedPluginId_;
    QString errorMessage_;
    QString statusMessage_;
    QVariantList servers_;
    QList<PlexAccount> accounts_;
    bool busy_ = false;
    bool connecting_ = false;
};

#endif // HAIKENANIME_PLUGINSETTINGSCONTROLLER_H
