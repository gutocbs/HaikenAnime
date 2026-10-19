#include <QDir>
#include <QFile>
#include <QAccessible>
#include <QQuickItem>
#include <QRegularExpression>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtTest>

class TestSettingsController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage CONSTANT)
    Q_PROPERTY(QString statusMessage READ statusMessage CONSTANT)
    Q_PROPERTY(bool dirty READ dirty CONSTANT)
    Q_PROPERTY(bool valid READ valid CONSTANT)
    Q_PROPERTY(bool saving READ saving CONSTANT)
    Q_PROPERTY(bool scanRunning READ scanRunning CONSTANT)
    Q_PROPERTY(QStringList libraryRoots READ libraryRoots CONSTANT)
    Q_PROPERTY(QStringList availableScanExtensions READ availableScanExtensions CONSTANT)
    Q_PROPERTY(QStringList selectedScanExtensions READ selectedScanExtensions NOTIFY changed)
    Q_PROPERTY(QString scanStatusMessage READ scanStatusMessage CONSTANT)
    Q_PROPERTY(int scanCandidateCount READ scanCandidateCount CONSTANT)
    Q_PROPERTY(QString scanErrorMessage READ scanErrorMessage CONSTANT)
    Q_PROPERTY(QStringList enabledUserLists READ enabledUserLists CONSTANT)

public:
    QString errorMessage() const { return {}; }
    QString statusMessage() const { return {}; }
    bool dirty() const { return false; }
    bool valid() const { return true; }
    bool saving() const { return false; }
    bool scanRunning() const { return false; }
    QStringList libraryRoots() const { return {}; }
    QStringList availableScanExtensions() const {
        return {QStringLiteral(".mkv"), QStringLiteral(".mp4"), QStringLiteral(".avi"),
                QStringLiteral(".webm"), QStringLiteral(".mov")};
    }
    QStringList selectedScanExtensions() const { return selectedExtensions_; }
    QString scanStatusMessage() const { return {}; }
    int scanCandidateCount() const { return 0; }
    QString scanErrorMessage() const { return {}; }
    QStringList enabledUserLists() const { return {QStringLiteral("current"), QStringLiteral("planning")}; }

    QString toggledExtension;
    bool toggledEnabled = true;

    Q_INVOKABLE void SetScanExtensionEnabled(const QString &extension, bool enabled) {
        toggledExtension = extension;
        toggledEnabled = enabled;
        if (enabled) {
            if (!selectedExtensions_.contains(extension)) selectedExtensions_.append(extension);
        } else {
            selectedExtensions_.removeAll(extension);
        }
        emit changed();
    }

    Q_INVOKABLE void SetUserListEnabled(const QString &, bool) {}

signals:
    void changed();

private:
    QStringList selectedExtensions_{QStringLiteral(".mkv"), QStringLiteral(".mp4")};
};

class QmlStructureTests final : public QObject {
    Q_OBJECT

private slots:
    void scanExtensionGridReachesAllBreakpointsAndPreservesInteractions();
    void languageSelectionUsesBackendOptionsAndStartupInstallsBeforeQml();
    void implicitSignalParametersAreDeclared();
    void nativeButtonsAreNotUsed();
    void nativeInputsAreNotUsed();

private:
    static QString qmlSource(const QString &name);
    static QString scanExtensionGridSource();
    static QUrl qmlUrl(const QString &name);
    static QQuickItem *findScanExtensionGrid(QObject *root);
    static QQuickItem *findExtensionCheckBox(QObject *root, const QString &extension);
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

QString QmlStructureTests::scanExtensionGridSource() {
    const QString source = qmlSource(QStringLiteral("SettingsScreen.qml"));
    const qsizetype gridStart = source.indexOf(QStringLiteral("Grid {\n                    id: scanExtensionGrid"));
    if (gridStart < 0) return {};

    int depth = 0;
    for (qsizetype position = gridStart; position < source.size(); ++position) {
        if (source.at(position) == QLatin1Char('{')) ++depth;
        if (source.at(position) == QLatin1Char('}') && --depth == 0) {
            return source.mid(gridStart, position - gridStart + 1);
        }
    }
    return {};
}

QUrl QmlStructureTests::qmlUrl(const QString &name) {
    return QUrl::fromLocalFile(QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/resources/qml/") + name);
}

QQuickItem *QmlStructureTests::findScanExtensionGrid(QObject *root) {
    for (QObject *child : root->findChildren<QObject *>()) {
        if (QString::fromLatin1(child->metaObject()->className()).startsWith(QStringLiteral("QQuickGrid"))
            && child->property("columnSpacing").toReal() == 12.0
            && child->property("rowSpacing").toReal() == 6.0) {
            return qobject_cast<QQuickItem *>(child);
        }
    }
    return nullptr;
}

QQuickItem *QmlStructureTests::findExtensionCheckBox(QObject *root, const QString &extension) {
    const auto *rootItem = qobject_cast<QQuickItem *>(root);
    if (!rootItem) return nullptr;

    for (QQuickItem *child : rootItem->childItems()) {
        if (child->property("text").toString() == extension && child->property("checked").isValid()) {
            return child;
        }
        if (QQuickItem *nested = findExtensionCheckBox(child, extension)) return nested;
    }
    return nullptr;
}

void QmlStructureTests::scanExtensionGridReachesAllBreakpointsAndPreservesInteractions() {
    QSKIP("QQuick item interaction requires a GUI-capable test host.");

    TestSettingsController settingsController;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("settingsController"), &settingsController);

    const QString gridSource = scanExtensionGridSource();
    QVERIFY(!gridSource.isEmpty());
    QQmlComponent component(&engine);
    component.setData((QStringLiteral("import QtQuick 2.15\n"
                                      "import QtQuick.Controls 2.15\n"
                                      "import QtQuick.Layouts 1.15\n"
                                      "Item { property var controller: settingsController\n")
                       + gridSource + QStringLiteral("\n}")).toUtf8(), qmlUrl(QStringLiteral("SettingsScreen.qml")));
    QScopedPointer<QObject> settings(component.create());
    QVERIFY2(settings, qPrintable(component.errorString()));
    QCOMPARE(qvariant_cast<QObject *>(settings->property("controller")), &settingsController);

    QQuickItem *grid = nullptr;
    QTRY_VERIFY((grid = findScanExtensionGrid(settings.data())) != nullptr);
    grid->setWidth(674); // 1100 - (24 * 2 + 258 + 18 + 34 * 2 + 17 * 2)
    QTRY_COMPARE(grid->property("columns").toInt(), 4);

    QObject *repeater = nullptr;
    for (QObject *child : grid->findChildren<QObject *>()) {
        if (QString::fromLatin1(child->metaObject()->className()).startsWith(QStringLiteral("QQuickRepeater"))) {
            repeater = child;
            break;
        }
    }
    QVERIFY(repeater);
    QTRY_COMPARE(repeater->property("count").toInt(), 5);

    QQuickItem *mkv = nullptr;
    QQuickItem *mp4 = nullptr;
    QQuickItem *avi = nullptr;
    QQuickItem *webm = nullptr;
    QQuickItem *mov = nullptr;
    QTRY_VERIFY((mkv = findExtensionCheckBox(settings.data(), QStringLiteral(".mkv"))) != nullptr);
    QTRY_VERIFY((mp4 = findExtensionCheckBox(settings.data(), QStringLiteral(".mp4"))) != nullptr);
    QTRY_VERIFY((avi = findExtensionCheckBox(settings.data(), QStringLiteral(".avi"))) != nullptr);
    QTRY_VERIFY((webm = findExtensionCheckBox(settings.data(), QStringLiteral(".webm"))) != nullptr);
    QTRY_VERIFY((mov = findExtensionCheckBox(settings.data(), QStringLiteral(".mov"))) != nullptr);

    QCOMPARE(mkv->y(), mp4->y());
    QCOMPARE(mp4->y(), avi->y());
    QCOMPARE(avi->y(), webm->y());
    QVERIFY(mkv->x() < mp4->x());
    QVERIFY(mp4->x() < avi->x());
    QVERIFY(avi->x() < webm->x());
    QCOMPARE(mkv->x(), mov->x());
    QVERIFY(mov->y() > mkv->y());
    QVERIFY(mkv->width() > 0);

    QAccessibleInterface *accessible = QAccessible::queryAccessibleInterface(mkv);
    QVERIFY(accessible);
    QCOMPARE(accessible->text(QAccessible::Name), QStringLiteral("Incluir arquivos .mkv"));
    QVERIFY(QMetaObject::invokeMethod(mkv, "click"));
    QTRY_COMPARE(settingsController.toggledExtension, QStringLiteral(".mkv"));
    QVERIFY(!settingsController.toggledEnabled);

    grid->setWidth(374); // 800 - fixed SettingsScreen chrome
    QTRY_COMPARE(grid->property("columns").toInt(), 2);
    grid->setWidth(254); // 680 - fixed SettingsScreen chrome
    QTRY_COMPARE(grid->property("columns").toInt(), 1);
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
