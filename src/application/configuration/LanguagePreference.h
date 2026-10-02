#ifndef HAIKENANIME_LANGUAGEPREFERENCE_H
#define HAIKENANIME_LANGUAGEPREFERENCE_H

#include <QString>
#include <QStringList>

inline QString DefaultLanguageKey() { return QStringLiteral("pt-BR"); }

inline QStringList SupportedLanguageKeys() {
    return {QStringLiteral("pt-BR"), QStringLiteral("en")};
}

inline bool IsSupportedLanguageKey(const QString &key) {
    return SupportedLanguageKeys().contains(key);
}

inline QString NormalizeLanguageKey(const QString &key) {
    return IsSupportedLanguageKey(key) ? key : DefaultLanguageKey();
}

#endif
