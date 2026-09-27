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
    Q_PROPERTY(QString libraryRoot READ libraryRoot CONSTANT)
    Q_PROPERTY(QStringList availableScanExtensions READ availableScanExtensions CONSTANT)
    Q_PROPERTY(QStringList selectedScanExtensions READ selectedScanExtensions NOTIFY changed)
    Q_PROPERTY(QString scanStatusMessage READ scanStatusMessage CONSTANT)
    Q_PROPERTY(int scanCandidateCount READ scanCandidateCount CONSTANT)
    Q_PROPERTY(QString scanErrorMessage READ scanErrorMessage CONSTANT)

public:
    QString errorMessage() const { return {}; }
    QString statusMessage() const { return {}; }
    bool dirty() const { return false; }
    bool valid() const { return true; }
    bool saving() const { return false; }
    bool scanRunning() const { return false; }
    QString libraryRoot() const { return {}; }
    QStringList availableScanExtensions() const {
        return {QStringLiteral(".mkv"), QStringLiteral(".mp4"), QStringLiteral(".avi"),
                QStringLiteral(".webm"), QStringLiteral(".mov")};
    }
    QStringList selectedScanExtensions() const { return selectedExtensions_; }
    QString scanStatusMessage() const { return {}; }
    int scanCandidateCount() const { return 0; }
    QString scanErrorMessage() const { return {}; }

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

signals:
    void changed();

private:
    QStringList selectedExtensions_{QStringLiteral(".mkv"), QStringLiteral(".mp4")};
};

class QmlStructureTests final : public QObject {
    Q_OBJECT

private slots:
    void scanExtensionGridUsesItsAvailableWidth();
    void settingsModeUsesNarrowMinimumWidth();
    void scanExtensionGridReachesAllBreakpointsAndPreservesInteractions();

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

void QmlStructureTests::scanExtensionGridUsesItsAvailableWidth() {
    QFile file(QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/resources/qml/SettingsScreen.qml"));
    QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(file.errorString()));

    const QString source = QString::fromUtf8(file.readAll());
    const qsizetype sectionStart = source.indexOf(QStringLiteral("EXTENSÕES INCLUÍDAS"));
    const qsizetype sectionEnd = source.indexOf(QStringLiteral("id: scanNowButton"), sectionStart);
    QVERIFY(sectionStart >= 0);
    QVERIFY(sectionEnd > sectionStart);

    const QString extensionSection = source.mid(sectionStart, sectionEnd - sectionStart);
    QVERIFY2(extensionSection.contains(QRegularExpression(
                 QStringLiteral(R"(Grid\s*\{[\s\S]*id\s*:\s*scanExtensionGrid)"))),
             "The extension choices must use a row-major Grid positioner.");
    QVERIFY2(extensionSection.contains(QRegularExpression(
                 QStringLiteral(R"(columns\s*:\s*width\s*>=\s*580\s*\?\s*4\s*:\s*width\s*>=\s*300\s*\?\s*2\s*:\s*1)"))),
             "The grid must derive four, two, or one columns from its own width.");
    QVERIFY2(extensionSection.contains(QRegularExpression(
                 QStringLiteral(R"(readonly\s+property\s+real\s+cellWidth\s*:[^\n]*\bwidth\b[^\n]*\bcolumns\b)"))),
             "The grid must calculate a cell width from its available width and column count.");
    QVERIFY2(extensionSection.contains(QRegularExpression(
                 QStringLiteral(R"(delegate\s*:\s*CheckBox\s*\{[\s\S]*width\s*:\s*scanExtensionGrid\.cellWidth)"))),
             "Each extension checkbox must be bounded to the grid cell width.");
}

void QmlStructureTests::settingsModeUsesNarrowMinimumWidth() {
    const QString mainSource = qmlSource(QStringLiteral("Main.qml"));
    QVERIFY(!mainSource.isEmpty());
    QVERIFY2(mainSource.contains(QStringLiteral("minimumWidth: root.showingSettings ? 680 : 1100")),
             "Settings must lower the application minimum width while Home keeps its 1100 px minimum.");
}

void QmlStructureTests::scanExtensionGridReachesAllBreakpointsAndPreservesInteractions() {
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

QTEST_MAIN(QmlStructureTests)
#include "QmlStructureTests.moc"
