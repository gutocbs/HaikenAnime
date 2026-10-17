#include "PluginSettingsService.h"

#include <QStringList>

#include <utility>

PluginSettingsService::PluginSettingsService(IPluginSettingsRepository &repository, IPluginSecretStore &secrets)
    : repository_(repository), secrets_(secrets) {}

QString PluginSettingsService::secretMask() {
    return QStringLiteral("********");
}

bool PluginSettingsService::registerSchema(const PluginSettingsSchema &schema, QString &error) {
    error.clear();
    if (schema.pluginId.trimmed().isEmpty() || plugins_.contains(schema.pluginId)) {
        error = QStringLiteral("Plugin '%1' is already registered or has no id.").arg(schema.pluginId);
        return false;
    }
    QVariantMap persisted;
    if (!repository_.read(schema.pluginId, persisted, error)) return false;

    PluginState state{.schema = schema};
    // Stored values that no longer match the schema fall back to defaults instead of blocking startup.
    for (auto entry = persisted.cbegin(); entry != persisted.cend(); ++entry) {
        const auto *field = schema.field(entry.key());
        if (field == nullptr || field->isSecret()) continue;
        QString ignored;
        if (const auto normalized = NormalizeSettingValue(*field, entry.value(), ignored)) {
            state.stored.insert(entry.key(), *normalized);
        }
    }
    order_.append(schema.pluginId);
    plugins_.insert(schema.pluginId, std::move(state));
    return true;
}

QList<PluginSettingsSchema> PluginSettingsService::schemas() const {
    QList<PluginSettingsSchema> result;
    result.reserve(order_.size());
    for (const auto &pluginId : order_) result.append(plugins_.value(pluginId).schema);
    return result;
}

QVariantMap PluginSettingsService::values(const QString &pluginId) const {
    const auto found = plugins_.constFind(pluginId);
    if (found == plugins_.cend()) return {};
    QVariantMap result;
    for (const auto &field : found->schema.fields) {
        if (field.isSecret()) {
            result.insert(field.key, hasSecret(pluginId, field.key) ? secretMask() : QString());
        } else if (found->draft.contains(field.key)) {
            result.insert(field.key, found->draft.value(field.key));
        } else if (found->stored.contains(field.key)) {
            result.insert(field.key, found->stored.value(field.key));
        } else {
            result.insert(field.key, field.defaultValue.isValid() ? field.defaultValue : QVariant(QString()));
        }
    }
    return result;
}

bool PluginSettingsService::hasSecret(const QString &pluginId, const QString &key) const {
    const auto found = plugins_.constFind(pluginId);
    if (found == plugins_.cend()) return false;
    return found->pendingSecrets.contains(key) || secrets_.hasSecret(pluginId, key);
}

bool PluginSettingsService::setValue(const QString &pluginId, const QString &key, const QVariant &value,
                                     QString &error) {
    error.clear();
    const auto found = plugins_.find(pluginId);
    if (found == plugins_.end()) {
        error = QStringLiteral("Plugin '%1' is not registered.").arg(pluginId);
        return false;
    }
    const auto *field = found->schema.field(key);
    if (field == nullptr) {
        error = QStringLiteral("Plugin '%1' has no setting '%2'.").arg(pluginId, key);
        return false;
    }
    const auto normalized = NormalizeSettingValue(*field, value, error);
    if (!normalized.has_value()) return false;
    if (field->isSecret()) {
        found->pendingSecrets.insert(key, normalized->toString());
    } else {
        found->draft.insert(key, *normalized);
    }
    return true;
}

bool PluginSettingsService::save(const QString &pluginId, QString &error) {
    error.clear();
    const auto found = plugins_.find(pluginId);
    if (found == plugins_.end()) {
        error = QStringLiteral("Plugin '%1' is not registered.").arg(pluginId);
        return false;
    }

    const auto effective = values(pluginId);
    QStringList missing;
    for (const auto &field : found->schema.fields) {
        if (!field.required) continue;
        const auto value = effective.value(field.key);
        const bool present = field.isSecret() ? hasSecret(pluginId, field.key)
                                              : value.isValid() && !value.toString().isEmpty();
        if (!present) missing.append(field.key);
    }
    if (!missing.isEmpty()) {
        error = QStringLiteral("Required settings are missing: %1.").arg(missing.join(QStringLiteral(", ")));
        return false;
    }

    for (auto secret = found->pendingSecrets.cbegin(); secret != found->pendingSecrets.cend(); ++secret) {
        QString storeError;
        if (!secrets_.writeSecret(pluginId, secret.key(), secret.value(), storeError)) {
            // The store message is dropped on purpose: it may echo the secret value.
            error = QStringLiteral("Could not store secret setting '%1' of '%2'.").arg(secret.key(), pluginId);
            return false;
        }
    }
    found->pendingSecrets.clear();

    auto merged = found->stored;
    for (auto entry = found->draft.cbegin(); entry != found->draft.cend(); ++entry) {
        merged.insert(entry.key(), entry.value());
    }
    if (!repository_.replace(pluginId, merged, error)) return false;
    found->stored = std::move(merged);
    found->draft.clear();
    return true;
}

bool PluginSettingsService::clearSecret(const QString &pluginId, const QString &key, QString &error) {
    error.clear();
    const auto found = plugins_.find(pluginId);
    const auto *field = found == plugins_.end() ? nullptr : found->schema.field(key);
    if (field == nullptr || !field->isSecret()) {
        error = QStringLiteral("Plugin '%1' has no secret setting '%2'.").arg(pluginId, key);
        return false;
    }
    QString storeError;
    if (!secrets_.clearSecret(pluginId, key, storeError)) {
        error = QStringLiteral("Could not clear secret setting '%1' of '%2'.").arg(key, pluginId);
        return false;
    }
    found->pendingSecrets.remove(key);
    return true;
}
