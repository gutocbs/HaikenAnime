#include "UserPreferencesValidator.h"

#include <QSet>

#include <cmath>

QStringList NormalizeScanExtensions(const QStringList &extensions, QString &error) {
    error.clear();
    if (extensions.isEmpty()) {
        error = QStringLiteral("At least one scan extension must be selected.");
        return {};
    }
    QStringList normalized;
    QSet<QString> seen;
    for (const auto &extension : extensions) {
        auto value = extension.trimmed().toLower();
        if (value.contains(QLatin1Char('/')) || value.contains(QLatin1Char('\\'))) {
            error = QStringLiteral("Scan extensions must not contain path separators.");
            return {};
        }
        while (value.startsWith(QLatin1Char('.'))) {
            value.remove(0, 1);
        }
        if (value.trimmed().isEmpty()) {
            error = QStringLiteral("Scan extensions must not be empty.");
            return {};
        }
        value.prepend(QLatin1Char('.'));
        if (seen.contains(value)) {
            error = QStringLiteral("Scan extensions must not contain duplicates.");
            return {};
        }
        seen.insert(value);
        normalized.append(value);
    }
    return normalized;
}

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
    if (preferences.cardStatusPresentation != CardStatusPresentation::PersonalListStatus
        && preferences.cardStatusPresentation != CardStatusPresentation::MediaReleaseStatus) {
        return {false, QStringLiteral("Card status presentation is unsupported.")};
    }
    if (!IsSupportedLanguageKey(preferences.languageKey)) {
        return {false, QStringLiteral("Language is unsupported.")};
    }
    if (preferences.synchronizationIntervalMs < MinimumSynchronizationIntervalMs
        || preferences.synchronizationIntervalMs > MaximumSynchronizationIntervalMs) {
        return {false, QStringLiteral("Synchronization interval is outside the supported range.")};
    }
    if (preferences.libraryRoot.trimmed().isEmpty()) {
        return {false, QStringLiteral("Library root must not be empty.")};
    }
    QString extensionError;
    if (NormalizeScanExtensions(preferences.scanExtensions, extensionError).isEmpty()) {
        return {false, extensionError};
    }
    return {true, {}};
}
