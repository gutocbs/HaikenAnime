#ifndef HAIKENANIME_MEDIATITLERESOLVER_H
#define HAIKENANIME_MEDIATITLERESOLVER_H

#include <QString>

#include "../../domain/media/Media.h"

inline const QString &DefaultPreferredTitleKey() {
    static const QString key = QStringLiteral("romaji");
    return key;
}

[[nodiscard]] bool IsSupportedPreferredTitleKey(const QString &key);
[[nodiscard]] QString NormalizePreferredTitleKey(const QString &key);
[[nodiscard]] QString ResolveMediaTitle(const Media &media, QString preferredTitleKey);

#endif
