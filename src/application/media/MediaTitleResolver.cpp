#include "MediaTitleResolver.h"

namespace {
bool hasText(const QString &value) {
    return !value.trimmed().isEmpty();
}
}

bool IsSupportedPreferredTitleKey(const QString &key) {
    return key == QStringLiteral("romaji") || key == QStringLiteral("english")
        || key == QStringLiteral("native");
}

QString NormalizePreferredTitleKey(const QString &key) {
    return IsSupportedPreferredTitleKey(key) ? key : DefaultPreferredTitleKey();
}

QString ResolveMediaTitle(const Media &media, QString preferredTitleKey) {
    preferredTitleKey = NormalizePreferredTitleKey(preferredTitleKey);
    const QString *preferred = &media.Name;
    if (preferredTitleKey == QStringLiteral("english")) preferred = &media.EnglishName;
    else if (preferredTitleKey == QStringLiteral("native")) preferred = &media.OriginalName;

    if (hasText(*preferred)) return *preferred;
    if (hasText(media.Name)) return media.Name;
    if (hasText(media.EnglishName)) return media.EnglishName;
    if (hasText(media.OriginalName)) return media.OriginalName;
    return media.Name;
}
