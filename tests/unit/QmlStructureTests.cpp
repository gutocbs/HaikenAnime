#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QtTest>

class QmlStructureTests final : public QObject {
    Q_OBJECT

private slots:
    void languageSelectionUsesBackendOptionsAndStartupInstallsBeforeQml();
    void implicitSignalParametersAreDeclared();
    void nativeButtonsAreNotUsed();
    void nativeInputsAreNotUsed();

private:
    static QString qmlSource(const QString &name);
};

QString QmlStructureTests::qmlSource(const QString &name) {
    QFile file(QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/resources/qml/") + name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QString::fromUtf8(file.readAll());
}

void QmlStructureTests::implicitSignalParametersAreDeclared() {
    const QDir directory(QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/resources/qml"));
    const QRegularExpression handlerStart(QStringLiteral(R"(\bon(?:Toggled|Activated|Accepted)\s*:\s*)"));
    const QRegularExpression implicitParameter(QStringLiteral(R"(\b(?:checked|index)\b)"));
    QStringList violations;

    for (const QString &fileName : directory.entryList({QStringLiteral("*.qml")}, QDir::Files)) {
        const QString source = qmlSource(fileName);
        QRegularExpressionMatchIterator matches = handlerStart.globalMatch(source);
        while (matches.hasNext()) {
            const QRegularExpressionMatch match = matches.next();
            const qsizetype bodyStart = match.capturedEnd();
            qsizetype bodyEnd = source.indexOf(QLatin1Char('\n'), bodyStart);
            if (bodyEnd < 0) bodyEnd = source.size();
            if (source.at(bodyStart) == QLatin1Char('{')) {
                int depth = 0;
                for (qsizetype position = bodyStart; position < source.size(); ++position) {
                    if (source.at(position) == QLatin1Char('{')) ++depth;
                    if (source.at(position) == QLatin1Char('}') && --depth == 0) {
                        bodyEnd = position + 1;
                        break;
                    }
                }
            }
            const QString body = source.mid(bodyStart, bodyEnd - bodyStart);
            if (body.startsWith(QStringLiteral("function")) || body.contains(QStringLiteral("=>"))) continue;
            if (!body.contains(implicitParameter)) continue;
            const qsizetype line = source.left(match.capturedStart()).count(QLatin1Char('\n')) + 1;
            violations << QStringLiteral("%1:%2").arg(fileName).arg(line);
        }
    }

    QVERIFY2(violations.isEmpty(), qPrintable(QStringLiteral("Handlers using undeclared implicit parameters: ")
                                              + violations.join(QStringLiteral(", "))));
}

void QmlStructureTests::nativeButtonsAreNotUsed() {
    const QRegularExpression nativeButton(QStringLiteral(R"((?:^|[^\w.])(?:Basic\.)?Button\s*\{)"),
                                          QRegularExpression::MultilineOption);
    const QRegularExpression delegateButton(QStringLiteral(R"(delegate\s*:\s*(?:Basic\.)?Button\s*\{)"));
    QStringList violations;
    for (const QString &fileName : {QStringLiteral("SettingsScreen.qml"), QStringLiteral("Home.qml"),
                                    QStringLiteral("EditMediaPanel.qml"), QStringLiteral("MediaDetailsPanel.qml"),
                                    QStringLiteral("SeasonalCatalogScreen.qml"), QStringLiteral("CoverPreview.qml")}) {
        const QString source = qmlSource(fileName);
        QVERIFY2(!source.isEmpty(), qPrintable(fileName));
        QRegularExpressionMatchIterator matches = nativeButton.globalMatch(source);
        while (matches.hasNext()) {
            const qsizetype line = source.left(matches.next().capturedStart()).count(QLatin1Char('\n')) + 1;
            violations << QStringLiteral("%1:%2").arg(fileName).arg(line);
        }
        if (delegateButton.match(source).hasMatch()) violations << fileName + QStringLiteral(":delegate");
        QVERIFY2(!source.contains(QStringLiteral("component SettingsButton")), qPrintable(fileName));
    }
    QVERIFY2(violations.isEmpty(), qPrintable(QStringLiteral("Native buttons still in use: ")
                                              + violations.join(QStringLiteral(", "))));
}

void QmlStructureTests::nativeInputsAreNotUsed() {
    // Unqualified controls resolve to the Windows style; only Basic.* or App* controls are styled.
    const QRegularExpression nativeInput(QStringLiteral(R"((?:^|[^\w.])(?:TextField|SpinBox|ComboBox)\s*\{)"),
                                         QRegularExpression::MultilineOption);
    QStringList violations;
    for (const QString &fileName : {QStringLiteral("SettingsScreen.qml"), QStringLiteral("PluginSettingField.qml"),
                                    QStringLiteral("SeasonalCatalogScreen.qml")}) {
        const QString source = qmlSource(fileName);
        QVERIFY2(!source.isEmpty(), qPrintable(fileName));
        QRegularExpressionMatchIterator matches = nativeInput.globalMatch(source);
        while (matches.hasNext()) {
            const qsizetype line = source.left(matches.next().capturedStart()).count(QLatin1Char('\n')) + 1;
            violations << QStringLiteral("%1:%2").arg(fileName).arg(line);
        }
    }
    QVERIFY2(violations.isEmpty(), qPrintable(QStringLiteral("Native inputs still in use: ")
                                              + violations.join(QStringLiteral(", "))));
    const QString cmakeSource = [] {
        QFile file(QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/CMakeLists.txt"));
        return file.open(QIODevice::ReadOnly | QIODevice::Text) ? QString::fromUtf8(file.readAll()) : QString();
    }();
    for (const QString &fileName : {QStringLiteral("AppTextField.qml"), QStringLiteral("AppSpinBox.qml"),
                                    QStringLiteral("AppComboBox.qml")}) {
        QVERIFY2(!qmlSource(fileName).isEmpty(), qPrintable(fileName));
        QVERIFY2(cmakeSource.contains(QStringLiteral("resources/qml/") + fileName), qPrintable(fileName));
    }
}

void QmlStructureTests::languageSelectionUsesBackendOptionsAndStartupInstallsBeforeQml() {
    const QString settingsSource = qmlSource(QStringLiteral("SettingsScreen.qml"));
    QVERIFY(settingsSource.contains(QStringLiteral("model: controller.languageOptions")));
    QVERIFY(settingsSource.contains(QStringLiteral("controller.languageKey")));
    QVERIFY(settingsSource.contains(QStringLiteral("controller.restartRequiredMessage")));
    QVERIFY(!settingsSource.contains(QStringLiteral("model: [\"pt-BR\", \"en\"]")));

    QFile mainFile(QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/main.cpp"));
    QVERIFY(mainFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString mainSource = QString::fromUtf8(mainFile.readAll());
    const auto installIndex = mainSource.indexOf(QStringLiteral("TranslationLoader::Install"));
    const auto engineIndex = mainSource.indexOf(QStringLiteral("QQmlApplicationEngine engine"));
    QVERIFY(installIndex >= 0);
    QVERIFY(engineIndex > installIndex);
}

QTEST_GUILESS_MAIN(QmlStructureTests)
#include "QmlStructureTests.moc"
