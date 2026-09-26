#ifndef HAIKENANIME_USERPREFERENCESVALIDATOR_H
#define HAIKENANIME_USERPREFERENCESVALIDATOR_H

#include <QString>

#include "UserPreferences.h"

inline constexpr int MinimumSynchronizationIntervalMs = 300000;
inline constexpr int MaximumSynchronizationIntervalMs = 86400000;

struct UserPreferencesValidationResult final {
    bool valid = false;
    QString error;
};

[[nodiscard]] UserPreferencesValidationResult ValidateUserPreferences(
    const UserPreferences &preferences);

// Trims and lowercases extensions, replacing leading periods with one period.
// Returns an empty list and sets error for empty selections/values, separators,
// or duplicates after normalization; clears error on success.
[[nodiscard]] QStringList NormalizeScanExtensions(const QStringList &extensions, QString &error);

#endif
