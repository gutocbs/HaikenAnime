#ifndef HAIKENANIME_TRANSLATIONLOADER_H
#define HAIKENANIME_TRANSLATIONLOADER_H

#include <QString>

class QCoreApplication;

class TranslationLoader final {
public:
    [[nodiscard]] static bool Install(QCoreApplication &application, QString languageKey,
                                      QString &error);
};

#endif
