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

#endif
