#ifndef HAIKENANIME_PLUGINSETTINGSSCHEMA_H
#define HAIKENANIME_PLUGINSETTINGSSCHEMA_H

#include <QList>
#include <QString>
#include <QVariant>

#include <optional>

enum class PluginSettingType { Text, Number, Boolean, Selection, Secret };

struct PluginSettingOption final {
    QString value;
    QString label;
};

/// Declarative description of one plugin setting, rendered generically by the presentation layer.
struct PluginSettingField final {
    QString key;
    PluginSettingType type = PluginSettingType::Text;
    QString label;
    QString description;
    bool required = false;
    QVariant defaultValue;
    std::optional<double> minimum;
    std::optional<double> maximum;
    QList<PluginSettingOption> options;

    /// Secret fields are write-only: their value never leaves the secret store.
    [[nodiscard]] bool isSecret() const { return type == PluginSettingType::Secret; }
};

struct PluginSettingsSchema final {
    QString pluginId;
    QString displayName;
    QList<PluginSettingField> fields;

    /// Returns the field with the given key, or nullptr when the schema does not declare it.
    [[nodiscard]] const PluginSettingField *field(const QString &key) const;
};

/// Validates a value against its field and returns the normalized value, or std::nullopt with
/// a message that names the field but never echoes the value.
[[nodiscard]] std::optional<QVariant> NormalizeSettingValue(const PluginSettingField &field,
                                                            const QVariant &value, QString &error);

#endif // HAIKENANIME_PLUGINSETTINGSSCHEMA_H
