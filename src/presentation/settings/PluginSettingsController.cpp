#include "PluginSettingsController.h"

#include <utility>

namespace {
const auto PlexId = QStringLiteral("plex");
const auto ServerKey = QStringLiteral("serverId");
const auto UsersKey = QStringLiteral("selectedUsers");

QString TypeName(const PluginSettingType type) {
    switch (type) {
    case PluginSettingType::Text: return QStringLiteral("text");
    case PluginSettingType::Number: return QStringLiteral("number");
    case PluginSettingType::Boolean: return QStringLiteral("boolean");
    case PluginSettingType::Selection: return QStringLiteral("selection");
    case PluginSettingType::Secret: return QStringLiteral("secret");
    }
    return QStringLiteral("text");
}
}

PluginSettingsController::PluginSettingsController(PluginSettingsService &settings, PluginRegistry &registry,
                                                   PlexAccountServices plex, std::function<void()> settingsApplied,
                                                   QObject *parent)
    : QObject(parent), settings_(settings), registry_(registry), plex_(plex),
      settingsApplied_(std::move(settingsApplied)) {
    const auto schemas = settings_.schemas();
    if (!schemas.isEmpty()) selectedPluginId_ = schemas.first().pluginId;
}

QVariantList PluginSettingsController::plugins() const {
    QVariantList result;
    for (const auto &schema : settings_.schemas()) {
        const auto *plugin = registry_.plugin(schema.pluginId);
        const auto capabilities = plugin ? plugin->capabilities() : PluginCapabilities();
        result.append(QVariantMap{
            {QStringLiteral("id"), schema.pluginId},
            {QStringLiteral("displayName"), schema.displayName},
            {QStringLiteral("browserAuthentication"), capabilities.testFlag(PluginCapability::BrowserAuthentication)},
            {QStringLiteral("connectionTest"), capabilities.testFlag(PluginCapability::ConnectionTest)},
            {QStringLiteral("userSelection"), capabilities.testFlag(PluginCapability::UserSelection)}});
    }
    return result;
}

QVariantList PluginSettingsController::fields() const {
    QVariantList result;
    const auto values = settings_.values(selectedPluginId_);
    for (const auto &schema : settings_.schemas()) {
        if (schema.pluginId != selectedPluginId_) continue;
        for (const auto &field : schema.fields) {
            // Server and users are chosen through dedicated actions, not typed by hand.
            if (selectedPluginId_ == PlexId && (field.key == ServerKey || field.key == UsersKey)) continue;
            QVariantList options;
            for (const auto &option : field.options) {
                options.append(QVariantMap{{QStringLiteral("value"), option.value}, {QStringLiteral("label"), option.label}});
            }
            result.append(QVariantMap{{QStringLiteral("key"), field.key},
                                      {QStringLiteral("type"), TypeName(field.type)},
                                      {QStringLiteral("label"), field.label},
                                      {QStringLiteral("description"), field.description},
                                      {QStringLiteral("required"), field.required},
                                      {QStringLiteral("value"), values.value(field.key)},
                                      {QStringLiteral("hasSecret"), field.isSecret() && settings_.hasSecret(selectedPluginId_, field.key)},
                                      {QStringLiteral("minimum"), field.minimum.value_or(0)},
                                      {QStringLiteral("maximum"), field.maximum.value_or(1'000'000)},
                                      {QStringLiteral("options"), options}});
        }
    }
    return result;
}

bool PluginSettingsController::dirty() const {
    return settings_.hasUnsavedChanges(selectedPluginId_);
}

QString PluginSettingsController::connectionState() const {
    if (!isPlexSelected() || plex_.authentication == nullptr) return QStringLiteral("disconnected");
    if (connecting_) return QStringLiteral("connecting");
    return plex_.authentication->hasToken() ? QStringLiteral("connected") : QStringLiteral("disconnected");
}

QVariantList PluginSettingsController::users() const {
    const auto selected = selectedUserIds();
    QVariantList result;
    for (const auto &account : accounts_) {
        result.append(QVariantMap{{QStringLiteral("id"), account.id},
                                  {QStringLiteral("name"), account.name},
                                  {QStringLiteral("selected"), selected.contains(account.id)}});
    }
    return result;
}

void PluginSettingsController::SelectPlugin(const QString &pluginId) {
    selectedPluginId_ = pluginId;
    servers_.clear();
    accounts_.clear();
    setMessages({}, {});
}

bool PluginSettingsController::SetValue(const QString &key, const QVariant &value) {
    QString error;
    const bool accepted = settings_.setValue(selectedPluginId_, key, value, error);
    setMessages(accepted ? QString() : error, {});
    return accepted;
}

bool PluginSettingsController::ClearSecret(const QString &key) {
    QString error;
    const bool cleared = settings_.clearSecret(selectedPluginId_, key, error);
    setMessages(cleared ? QString() : error, cleared ? tr("Secret removed.") : QString());
    return cleared;
}

bool PluginSettingsController::Save() {
    QString error;
    if (!settings_.save(selectedPluginId_, error)) {
        setMessages(error, {});
        return false;
    }
    if (settingsApplied_) settingsApplied_();
    setMessages({}, tr("Settings saved."));
    return true;
}

void PluginSettingsController::Discard() {
    settings_.discard(selectedPluginId_);
    setMessages({}, {});
}

void PluginSettingsController::Connect() {
    if (!isPlexSelected() || plex_.authentication == nullptr) return;
    connecting_ = true;
    setMessages({}, tr("Finish the Plex login in your browser."));
    plex_.authentication->beginBrowserLogin([this](const PlexLoginResult &result) {
        connecting_ = false;
        const bool succeeded = result.status == PlexLoginStatus::Succeeded;
        setMessages(succeeded ? QString() : result.safeMessage, succeeded ? result.safeMessage : QString());
    });
}

void PluginSettingsController::Disconnect() {
    if (!isPlexSelected() || plex_.authentication == nullptr) return;
    QString error;
    const bool disconnected = plex_.authentication->disconnect(error);
    if (disconnected && plex_.plugin) plex_.plugin->stop();
    servers_.clear();
    accounts_.clear();
    setMessages(disconnected ? QString() : error, disconnected ? tr("Disconnected from Plex.") : QString());
}

void PluginSettingsController::TestConnection() {
    auto *plugin = registry_.plugin(selectedPluginId_);
    if (plugin == nullptr) return;
    busy_ = true;
    emit changed();
    plugin->testConnection([this](const ConnectionTestResult &result) {
        busy_ = false;
        setMessages(result.succeeded ? QString() : result.safeMessage, result.succeeded ? result.safeMessage : QString());
    });
}

void PluginSettingsController::LoadServers() {
    if (!isPlexSelected() || plex_.authentication == nullptr || plex_.api == nullptr) return;
    QString token;
    QString error;
    if (!plex_.authentication->readToken(token, error)) {
        setMessages(error, {});
        return;
    }
    busy_ = true;
    emit changed();
    plex_.api->discoverServers(token, [this](const PlexApiError result, const QList<PlexServer> &servers) {
        busy_ = false;
        servers_.clear();
        const auto selected = settings_.values(PlexId).value(ServerKey).toString();
        for (const auto &server : servers) {
            servers_.append(QVariantMap{{QStringLiteral("id"), server.clientIdentifier},
                                        {QStringLiteral("name"), server.name},
                                        {QStringLiteral("owned"), server.owned},
                                        {QStringLiteral("selected"), server.clientIdentifier == selected}});
        }
        setMessages(result == PlexApiError::None ? QString() : tr("Plex servers could not be listed."), {});
    });
}

void PluginSettingsController::SelectServer(const QString &serverId) {
    if (!isPlexSelected() || plex_.plugin == nullptr) return;
    plex_.plugin->selectServer(serverId, [this, serverId](const bool selected, const QString &message) {
        if (!selected) {
            setMessages(message, {});
            return;
        }
        QString error;
        settings_.setValue(PlexId, ServerKey, serverId, error);
        for (auto &entry : servers_) {
            auto server = entry.toMap();
            server.insert(QStringLiteral("selected"), server.value(QStringLiteral("id")).toString() == serverId);
            entry = server;
        }
        accounts_.clear();
        setMessages(error, {});
    });
}

void PluginSettingsController::LoadUsers() {
    if (!isPlexSelected() || plex_.plugin == nullptr || plex_.api == nullptr || !plex_.plugin->server()) return;
    busy_ = true;
    emit changed();
    plex_.api->listUsers(*plex_.plugin->server(), [this](const PlexApiError result, const QList<PlexAccount> &accounts) {
        busy_ = false;
        accounts_ = accounts;
        setMessages(result == PlexApiError::None ? QString() : tr("Plex users could not be listed."), {});
    });
}

void PluginSettingsController::SetUserSelected(const QString &userId, const bool selected) {
    auto ids = selectedUserIds();
    ids.removeAll(userId);
    if (selected) ids.append(userId);
    QString error;
    settings_.setValue(PlexId, UsersKey, ids.join(QLatin1Char(',')), error);
    setMessages(error, {});
}

void PluginSettingsController::setMessages(const QString &error, const QString &status) {
    errorMessage_ = error;
    statusMessage_ = status;
    emit changed();
}

bool PluginSettingsController::isPlexSelected() const {
    return selectedPluginId_ == PlexId;
}

QStringList PluginSettingsController::selectedUserIds() const {
    return settings_.values(PlexId).value(UsersKey).toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
}
