#include "PluginSettingsSchema.h"

#include <algorithm>

namespace {
bool IsNumeric(const QVariant &value) {
    switch (value.typeId()) {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Double:
    case QMetaType::Float:
        return true;
    default:
        return false;
    }
}
}

const PluginSettingField *PluginSettingsSchema::field(const QString &key) const {
    const auto found = std::find_if(fields.cbegin(), fields.cend(),
                                    [&key](const PluginSettingField &candidate) { return candidate.key == key; });
    return found == fields.cend() ? nullptr : &*found;
}

std::optional<QVariant> NormalizeSettingValue(const PluginSettingField &field, const QVariant &value,
                                              QString &error) {
    error.clear();
    // Messages name the field only: the value may be a secret and must never be echoed.
    switch (field.type) {
    case PluginSettingType::Text:
        if (value.typeId() != QMetaType::QString) break;
        return QVariant(value.toString().trimmed());
    case PluginSettingType::Secret:
        if (value.typeId() != QMetaType::QString || value.toString().isEmpty()) break;
        return value;
    case PluginSettingType::Boolean:
        if (value.typeId() != QMetaType::Bool) break;
        return value;
    case PluginSettingType::Number: {
        if (!IsNumeric(value)) break;
        const auto number = value.toDouble();
        if ((field.minimum.has_value() && number < *field.minimum)
            || (field.maximum.has_value() && number > *field.maximum)) {
            error = QStringLiteral("Setting '%1' is outside the allowed range.").arg(field.key);
            return std::nullopt;
        }
        return value;
    }
    case PluginSettingType::Selection: {
        if (value.typeId() != QMetaType::QString) break;
        const auto selected = value.toString();
        const bool known = std::any_of(field.options.cbegin(), field.options.cend(),
                                       [&selected](const PluginSettingOption &option) {
                                           return option.value == selected;
                                       });
        if (!known) {
            error = QStringLiteral("Setting '%1' must be one of the listed options.").arg(field.key);
            return std::nullopt;
        }
        return value;
    }
    }
    error = QStringLiteral("Setting '%1' has an invalid value type.").arg(field.key);
    return std::nullopt;
}
