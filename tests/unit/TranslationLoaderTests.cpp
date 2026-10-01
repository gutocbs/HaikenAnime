#include <QCoreApplication>
#include <QDir>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QtTest>

#include "../../src/app/TranslationLoader.h"

#include <memory>

class TranslationLoaderTests final : public QObject {
    Q_OBJECT

private slots:
    void installsPortugueseCatalog();
    void installsEnglishCatalog();
    void installsEnglishCatalogBeforeQmlComponentCreation();
    void invalidKeyFallsBackToPortuguese();
    void missingCatalogReportsErrorAndKeepsDefaultLanguage();
};

void TranslationLoaderTests::installsPortugueseCatalog() {
    QString error;
    QVERIFY2(TranslationLoader::Install(*QCoreApplication::instance(), QStringLiteral("pt-BR"), error),
             qPrintable(error));
    QVERIFY(error.isEmpty());
    QCOMPARE(QCoreApplication::translate("SettingsController", "Alterações salvas."),
             QStringLiteral("Alterações salvas."));
}

void TranslationLoaderTests::installsEnglishCatalog() {
    QString error;
    QVERIFY2(TranslationLoader::Install(*QCoreApplication::instance(), QStringLiteral("en"), error),
             qPrintable(error));
    QVERIFY(error.isEmpty());
    QCOMPARE(QCoreApplication::translate("SettingsController", "Alterações salvas."),
             QStringLiteral("Changes saved."));
}

void TranslationLoaderTests::installsEnglishCatalogBeforeQmlComponentCreation() {
    QString error;
    QVERIFY2(TranslationLoader::Install(*QCoreApplication::instance(), QStringLiteral("en"), error),
             qPrintable(error));

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQml\n"
                      "QtObject { property string text: qsTranslate(\"SettingsController\", "
                      "\"Altera\303\247\303\265es salvas.\") }",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QCOMPARE(root->property("text").toString(), QStringLiteral("Changes saved."));
}

void TranslationLoaderTests::invalidKeyFallsBackToPortuguese() {
    QString error;
    QVERIFY2(TranslationLoader::Install(*QCoreApplication::instance(), QStringLiteral("obsolete"), error),
             qPrintable(error));
    QVERIFY(error.isEmpty());
    QCOMPARE(QCoreApplication::translate("SettingsController", "Alterações salvas."),
             QStringLiteral("Alterações salvas."));
}

void TranslationLoaderTests::missingCatalogReportsErrorAndKeepsDefaultLanguage() {
    const auto originalOverride = qgetenv("HAIKENANIME_TRANSLATIONS_DIR");
    qputenv("HAIKENANIME_TRANSLATIONS_DIR", QDir::tempPath().toUtf8() + "/missing-haikenanime-catalogs");
    QString error;
    QVERIFY(!TranslationLoader::Install(*QCoreApplication::instance(), QStringLiteral("en"), error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(QCoreApplication::translate("SettingsController", "Alterações salvas."),
             QStringLiteral("Alterações salvas."));
    if (originalOverride.isNull()) qunsetenv("HAIKENANIME_TRANSLATIONS_DIR");
    else qputenv("HAIKENANIME_TRANSLATIONS_DIR", originalOverride);
}

QTEST_GUILESS_MAIN(TranslationLoaderTests)
#include "TranslationLoaderTests.moc"
