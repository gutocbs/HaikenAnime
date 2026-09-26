#include "UserPreferencesValidator.h"

#include <cmath>

UserPreferencesValidationResult ValidateUserPreferences(const UserPreferences &preferences) {
    const auto finite = [](const double value) { return std::isfinite(value); };
    if (!finite(preferences.scoreMinimum) || !finite(preferences.scoreMaximum)
        || !finite(preferences.scoreStep)) {
        return {false, QStringLiteral("Score values must be finite.")};
    }
    const double range = preferences.scoreMaximum - preferences.scoreMinimum;
    if (range <= 0.0) {
        return {false, QStringLiteral("Score maximum must be greater than the minimum.")};
    }
    if (preferences.scoreStep <= 0.0 || preferences.scoreStep > range) {
        return {false, QStringLiteral("Score step must fit inside the configured range.")};
    }
    const double steps = range / preferences.scoreStep;
    if (std::abs(steps - std::round(steps)) > 1e-9) {
        return {false, QStringLiteral("Score range must be divisible by the configured step.")};
    }
    if (preferences.coverQuality != CoverQuality::Medium
        && preferences.coverQuality != CoverQuality::Large
        && preferences.coverQuality != CoverQuality::ExtraLarge) {
        return {false, QStringLiteral("Cover quality is unsupported.")};
    }
    if (preferences.synchronizationIntervalMs < MinimumSynchronizationIntervalMs
        || preferences.synchronizationIntervalMs > MaximumSynchronizationIntervalMs) {
        return {false, QStringLiteral("Synchronization interval is outside the supported range.")};
    }
    return {true, {}};
}
