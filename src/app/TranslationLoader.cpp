#include "TranslationLoader.h"

#include <QCoreApplication>
#include <QDir>
#include <QTranslator>

#include "../application/configuration/LanguagePreference.h"

#include <memory>

namespace {
std::unique_ptr<QTranslator> installedTranslator;

QString catalogDirectory() {
    const auto overrideDirectory = qEnvironmentVariable("HAIKENANIME_TRANSLATIONS_DIR");
    return overrideDirectory.isEmpty() ? QStringLiteral(":/i18n") : overrideDirectory;
}
}

bool TranslationLoader::Install(QCoreApplication &application, QString languageKey,
                                QString &error) {
    error.clear();
    languageKey = NormalizeLanguageKey(languageKey);

    if (installedTranslator) {
        application.removeTranslator(installedTranslator.get());
        installedTranslator.reset();
    }

    auto translator = std::make_unique<QTranslator>();
    const auto catalogName = QStringLiteral("HaikenAnime_%1").arg(
        languageKey == QStringLiteral("pt-BR") ? QStringLiteral("pt_BR") : languageKey);
    const auto directory = catalogDirectory();
    if (!translator->load(catalogName, directory)) {
        error = QStringLiteral("Could not load translation catalog '%1' from '%2'.")
                    .arg(catalogName, directory);
        return false;
    }
    if (!application.installTranslator(translator.get())) {
        error = QStringLiteral("Could not install translation catalog '%1'.").arg(catalogName);
        return false;
    }
    installedTranslator = std::move(translator);
    return true;
}
