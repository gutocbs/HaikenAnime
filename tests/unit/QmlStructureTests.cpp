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
    void browseControlsShowConfiguredLabelsAfterInitialization();
    void mediaCardAndSettingsUseControllerPreparedCardPresentation();
    void previewCardsUseControllerPreparedMetadataInBothGrids();
    void compactDetailsUseSeparatePreviewMetadata();
    void compactDetailsAreReadOnlyAndSelectable();
    void coverPreviewReusesSelectedCoverSourceWithoutRequestingDownloads();
    void languageSelectionUsesBackendOptionsAndStartupInstallsBeforeQml();
    void preferredTitleSelectionUsesBackendOptions();
    void seasonalCatalogRequiresExplicitFiltersAndHasResponsiveContent();
    void seasonalDetailsKeepAddAndEditFlowsExplicitAndStatusGated();
    void mainKeepsSeasonalCatalogAsSeparateNavigation();

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

void QmlStructureTests::browseControlsShowConfiguredLabelsAfterInitialization() {
    const QString source = qmlSource(QStringLiteral("BrowseControls.qml"));
    QVERIFY(!source.isEmpty());
    QVERIFY2(source.contains(QStringLiteral(
                 "currentIndex: count > 0 ? indexOfValue(controls.controller.activeListFilter) : -1")),
             "List selection must re-evaluate when backend-provided options arrive.");
    QVERIFY2(source.contains(QStringLiteral(
                 "currentIndex: count > 0 ? indexOfValue(controls.controller.activeSort) : -1")),
             "Sort selection must re-evaluate when backend-provided options arrive.");
    QVERIFY(!source.contains(QStringLiteral("currentIndex: 0")));
}

void QmlStructureTests::mediaCardAndSettingsUseControllerPreparedCardPresentation() {
    const QString cardSource = qmlSource(QStringLiteral("MediaCard.qml"));
    const QString settingsSource = qmlSource(QStringLiteral("SettingsScreen.qml"));
    QVERIFY(!cardSource.isEmpty());
    QVERIFY(!settingsSource.isEmpty());

    QVERIFY(cardSource.contains(QStringLiteral("text: card.status")));
    QVERIFY(cardSource.contains(QStringLiteral("text: card.progress")));
    QVERIFY(cardSource.contains(QStringLiteral("text: card.score")));
    QVERIFY(!cardSource.contains(QStringLiteral("Minha lista:")));
    QVERIFY(!cardSource.contains(QStringLiteral("Exibição:")));
    QVERIFY(!cardSource.contains(QStringLiteral("Progresso ")));
    QVERIFY(!cardSource.contains(QStringLiteral("Nota ")));
    QVERIFY(settingsSource.contains(QStringLiteral("model: controller.cardStatusPresentationOptions")));
    QVERIFY(settingsSource.contains(QStringLiteral("controller.cardStatusPresentationKey")));
    QVERIFY(!settingsSource.contains(QStringLiteral("personal-list-status")));
    QVERIFY(!settingsSource.contains(QStringLiteral("media-release-status")));
}

void QmlStructureTests::previewCardsUseControllerPreparedMetadataInBothGrids() {
    const QString source = qmlSource(QStringLiteral("Home.qml"));
    QVERIFY(!source.isEmpty());

    QVERIFY(source.contains(QStringLiteral(
        "const cardMetadata = controller.PreviewCardMetadata(progress, statusKey, score)")));
    QVERIFY(!source.contains(QStringLiteral("function statusLabel(")));
    QVERIFY(!source.contains(QStringLiteral("progress + \"/\"")));
    QVERIFY(!source.contains(QStringLiteral("score === 0 ? \"—\"")));
    QCOMPARE(source.count(QStringLiteral("status: home.previewValue(model.mediaId, \"cardStatusText\", model.statusLabel)")), 2);
    QCOMPARE(source.count(QStringLiteral("progress: home.previewValue(model.mediaId, \"cardProgressText\", model.progress)")), 2);
    QCOMPARE(source.count(QStringLiteral("score: home.previewValue(model.mediaId, \"cardScoreText\", model.score)")), 2);
}

void QmlStructureTests::compactDetailsUseSeparatePreviewMetadata() {
    const QString source = qmlSource(QStringLiteral("Home.qml"));
    QVERIFY(!source.isEmpty());

    QVERIFY(source.contains(QStringLiteral(
        "const compactDetailMetadata = controller.PreviewCompactDetailMetadata(progress, statusKey, score)")));
    QVERIFY(source.contains(QStringLiteral(
        "status: home.previewValue(model.mediaId, \"cardStatusText\", model.statusLabel)")));
    QVERIFY(source.contains(QStringLiteral(
        "progress: home.previewValue(model.mediaId, \"cardProgressText\", model.progress)")));
    QVERIFY(source.contains(QStringLiteral(
        "score: home.previewValue(model.mediaId, \"cardScoreText\", model.score)")));
    QVERIFY(source.contains(QStringLiteral(
        "home.previewValue(controller.selectedMediaId, \"detailStatusText\",")));
    QVERIFY(source.contains(QStringLiteral(
        "home.previewValue(controller.selectedMediaId, \"detailProgressText\",")));
    QVERIFY(source.contains(QStringLiteral(
        "home.previewValue(controller.selectedMediaId, \"detailScoreText\",")));
}

void QmlStructureTests::compactDetailsAreReadOnlyAndSelectable() {
    const QString source = qmlSource(QStringLiteral("Home.qml"));
    QVERIFY(!source.isEmpty());

    const qsizetype detailsStart = source.indexOf(QStringLiteral("visible: controller.hasSelection"));
    const qsizetype detailsEnd = source.indexOf(QStringLiteral("EditMediaPanel {"), detailsStart);
    QVERIFY(detailsStart >= 0);
    QVERIFY(detailsEnd > detailsStart);
    const QString details = source.mid(detailsStart, detailsEnd - detailsStart);

    for (const QString &controlId : {QStringLiteral("selectedTitleText"),
                                    QStringLiteral("selectedTypeText"),
                                    QStringLiteral("selectedStatusText"),
                                    QStringLiteral("selectedProgressText"),
                                    QStringLiteral("selectedScoreText")}) {
        QVERIFY2(details.contains(QRegularExpression(
                     QStringLiteral(R"(TextEdit\s*\{[^}]*id\s*:\s*)") + controlId
                     + QStringLiteral(R"([^}]*readOnly\s*:\s*true[^}]*selectByMouse\s*:\s*true)"))),
                 qPrintable(controlId + QStringLiteral(" must remain read-only and selectable by mouse.")));
    }

    QVERIFY2(details.contains(QRegularExpression(
                 QStringLiteral(R"(id\s*:\s*selectedTitleText[^}]*wrapMode\s*:\s*TextEdit\.Wrap)"))),
             "The selected title must preserve wrapped presentation.");
    QVERIFY2(details.contains(QRegularExpression(
                 QStringLiteral(R"(TextEdit\s*\{[^}]*id\s*:\s*selectedSynopsisText[^}]*readOnly\s*:\s*true[^}]*selectByMouse\s*:\s*true[^}]*wrapMode\s*:\s*TextEdit\.Wrap[^}]*verticalAlignment\s*:\s*TextEdit\.AlignTop[^}]*clip\s*:\s*true)"))),
             "The selected synopsis must use a read-only selectable TextEdit and keep its wrapped, top-aligned clipped boundary.");
    QVERIFY(!details.contains(QRegularExpression(
        QStringLiteral(R"(Text\s*\{[^}]*id\s*:\s*selectedSynopsisText)"))));
    QVERIFY(!details.contains(QRegularExpression(
        QStringLiteral(R"(id\s*:\s*selectedSynopsisText[^}]*elide\s*:)"))));
    QVERIFY(details.contains(QStringLiteral("id: selectedSynopsisOverflowIndicator")));
    QVERIFY(details.contains(QStringLiteral("visible: selectedSynopsisText.contentHeight > selectedSynopsisText.height")));
    QVERIFY(details.contains(QStringLiteral("text: \"…\"")));
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

void QmlStructureTests::preferredTitleSelectionUsesBackendOptions() {
    const QString settingsSource = qmlSource(QStringLiteral("SettingsScreen.qml"));
    QVERIFY(settingsSource.contains(QStringLiteral("model: controller.preferredTitleOptions")));
    QVERIFY(settingsSource.contains(QStringLiteral("controller.preferredTitleKey")));
    QVERIFY(settingsSource.contains(QStringLiteral("controller.SetPreferredTitle")));
    QVERIFY(!settingsSource.contains(QStringLiteral("model: [\"romaji\", \"english\", \"native\"]")));
    QVERIFY(settingsSource.contains(QStringLiteral("controller.includeAdultContent")));
    QVERIFY(settingsSource.contains(QStringLiteral("controller.SetIncludeAdultContent")));
    QVERIFY(settingsSource.contains(QStringLiteral("INCLUIR CONTEÚDO ADULTO")));
}

void QmlStructureTests::coverPreviewReusesSelectedCoverSourceWithoutRequestingDownloads() {
    const QString homeSource = qmlSource(QStringLiteral("Home.qml"));
    const QString previewSource = qmlSource(QStringLiteral("CoverPreview.qml"));
    QVERIFY(!homeSource.isEmpty());
    QVERIFY2(!previewSource.isEmpty(), "Cover preview must be a dedicated QML component.");

    QVERIFY2(homeSource.contains(QStringLiteral("CoverPreview {")),
             "Home must compose the local cover preview component.");
    QVERIFY2(homeSource.contains(QStringLiteral("source: controller.selectedCoverSource")),
             "Home must pass only the selected cover source to the preview.");
    QVERIFY2(previewSource.contains(QStringLiteral("property url source")),
             "The preview must expose the selected cover source as its input.");
    QVERIFY2(previewSource.contains(QStringLiteral("source: preview.source")),
             "The preview image must use the supplied selected cover source.");
    QVERIFY2(previewSource.contains(QStringLiteral("fillMode: Image.PreserveAspectFit")),
             "The preview must preserve portrait, landscape, wide, and narrow image proportions.");
    QVERIFY2(previewSource.contains(QStringLiteral("modal: true"))
                 && previewSource.contains(QStringLiteral("focus: true")),
             "The preview must be a focus-containing modal surface.");
    QVERIFY2(previewSource.contains(QStringLiteral("Keys.onTabPressed"))
                 && previewSource.contains(QStringLiteral("Keys.onBacktabPressed"))
                 && previewSource.count(QStringLiteral("closeButton.forceActiveFocus()")) >= 3,
             "The preview must explicitly cycle Tab and Shift+Tab focus within the modal.");
    QVERIFY2(previewSource.contains(QStringLiteral("Popup.CloseOnEscape | Popup.CloseOnPressOutside")),
             "The preview must close on Escape and backdrop press.");
    QVERIFY2(previewSource.contains(QStringLiteral("returnFocusItem.forceActiveFocus()")),
             "The preview must restore focus to the invoking cover after close.");
    QVERIFY2(homeSource.contains(QStringLiteral("coverPreview.openForSource(coverTrigger)")),
             "The selected cover must open its local preview.");
    QVERIFY2(previewSource.contains(QStringLiteral("image.status === Image.Error")),
             "A missing local cover must remain a non-fatal preview state.");

    QVERIFY2(!previewSource.contains(QStringLiteral("RequestCoverWindow")),
             "Opening the preview must not request a cover download window.");
    QVERIFY2(!previewSource.contains(QStringLiteral("RequestCover")),
             "Opening the preview must not request cover downloads.");
    QVERIFY2(!previewSource.contains(QStringLiteral("CoverDownload")),
             "The preview must not depend on the cover download controller.");
}

void QmlStructureTests::seasonalCatalogRequiresExplicitFiltersAndHasResponsiveContent() {
    const QString source = qmlSource(QStringLiteral("SeasonalCatalogScreen.qml"));
    const QString detailsSource = qmlSource(QStringLiteral("MediaDetailsPanel.qml"));
    QVERIFY2(!source.isEmpty(), "The seasonal catalog needs its own screen.");
    QVERIFY(!detailsSource.isEmpty());
    QVERIFY(source.contains(QStringLiteral("currentIndex: controller.selectedYear > 0")));
    QVERIFY(source.contains(QStringLiteral("currentIndex: controller.selectedSeasonKey.length > 0")));
    QVERIFY(source.contains(QStringLiteral("controller.SetYear")));
    QVERIFY(source.contains(QStringLiteral("controller.SetSeason")));
    QVERIFY(source.contains(QStringLiteral("controller.Retry()")));
    QVERIFY(source.contains(QStringLiteral("controller.LoadNextPage()")));
    QVERIFY(source.contains(QStringLiteral("function requestNextPageIfNearEnd()")));
    QVERIFY(source.contains(QStringLiteral("if (!controller.canLoadNextPage) return")));
    QVERIFY(source.contains(QStringLiteral("onContentYChanged: requestNextPageIfNearEnd()")));
    QVERIFY(source.contains(QStringLiteral("onContentHeightChanged: requestNextPageIfNearEnd()")));
    QVERIFY(source.contains(QStringLiteral("id: nextPageLoadingIndicator")));
    QVERIFY(source.contains(QStringLiteral("id: initialLoadingState")));
    QVERIFY(source.contains(QStringLiteral("id: emptyState")));
    QVERIFY(source.contains(QStringLiteral("id: errorState")));
    QVERIFY(source.count(QStringLiteral("anchors.centerIn: parent")) >= 3);
    QVERIFY(source.contains(QStringLiteral("id: seasonalErrorRetryButton")));
    QVERIFY(source.contains(QStringLiteral("text: controller.errorMessage")));
    const qsizetype errorRetryButton = source.indexOf(QStringLiteral("id: seasonalErrorRetryButton"));
    QVERIFY(source.indexOf(QStringLiteral("onClicked: controller.Retry()"), errorRetryButton)
        > errorRetryButton);
    QVERIFY(source.contains(QStringLiteral("visible: controller.state === \"loading\" && controller.hasResults")));
    QVERIFY(!source.contains(QStringLiteral("Carregar próxima página")));
    QVERIFY(source.contains(QStringLiteral("columns: width >= 860 ? 3 : width >= 560 ? 2 : 1")));
    QVERIFY(source.contains(QStringLiteral("Flow {")));
    QVERIFY(source.contains(QStringLiteral("width >= 560")));
    QVERIFY(source.contains(QStringLiteral("width: filterLayout.width >= 560 ? 150 : filterLayout.width")));
    QVERIFY(source.contains(QStringLiteral("width: filterLayout.width >= 560 ? 170 : filterLayout.width")));
    QVERIFY(source.contains(QStringLiteral("MediaDetailsPanel {")));
    QVERIFY(source.contains(QStringLiteral("seasonalLayout: true")));
    QVERIFY(source.contains(QStringLiteral("EditMediaPanel {")));

    QVERIFY(detailsSource.contains(QStringLiteral("property bool seasonalLayout: false")));
    QVERIFY(detailsSource.contains(QStringLiteral("id: seasonalDetailsLayout")));
    QVERIFY(detailsSource.contains(QStringLiteral("id: seasonalMetadata")));
    QVERIFY(detailsSource.contains(QStringLiteral("id: seasonalCover")));
    QVERIFY(detailsSource.indexOf(QStringLiteral("id: seasonalMetadata"))
        < detailsSource.indexOf(QStringLiteral("id: seasonalCover")));
    QVERIFY(detailsSource.contains(QStringLiteral("visible: panel.seasonalLayout")));
    QVERIFY(detailsSource.contains(QStringLiteral("source: controller.selectedCoverSource")));
    QVERIFY(detailsSource.contains(QStringLiteral("id: externalLinksGrid")));
    QVERIFY(detailsSource.contains(QStringLiteral("columns: width >= 340 ? 2 : 1")));
}

void QmlStructureTests::seasonalDetailsKeepAddAndEditFlowsExplicitAndStatusGated() {
    const QString seasonalSource = qmlSource(QStringLiteral("SeasonalCatalogScreen.qml"));
    const QString detailsSource = qmlSource(QStringLiteral("MediaDetailsPanel.qml"));
    const QString editorSource = qmlSource(QStringLiteral("EditMediaPanel.qml"));

    QVERIFY(seasonalSource.contains(QStringLiteral("EditMediaPanel {")));
    QVERIFY(seasonalSource.contains(QStringLiteral(
        "controller.SaveSelectedToPersonalListFromEditor(progress, statusKey, score, path, alternativeNames)")));
    QVERIFY(seasonalSource.contains(QStringLiteral(
        "if (controller.SaveSelectedToPersonalListFromEditor(progress, statusKey, score, path, alternativeNames))")));
    QVERIFY(seasonalSource.contains(QStringLiteral("requiresExplicitStatus: true")));
    QVERIFY(detailsSource.contains(QStringLiteral("id: addToMyListButton")));
    QVERIFY(detailsSource.contains(QStringLiteral("text: qsTr(\"Adicionar à minha lista\")")));
    QVERIFY(detailsSource.contains(QStringLiteral("visible: panel.seasonalLayout && !controller.selectedMediaInPersonalList")));
    QVERIFY(detailsSource.contains(QStringLiteral("id: editPersonalListButton")));
    QVERIFY(detailsSource.contains(QStringLiteral("visible: panel.seasonalLayout && controller.selectedMediaInPersonalList")));
    QVERIFY(editorSource.contains(QStringLiteral("property bool requiresExplicitStatus: false")));
    QVERIFY(editorSource.contains(QStringLiteral("enabled: !panel.requiresExplicitStatus || statusField.currentIndex >= 0")));
    QVERIFY(editorSource.contains(QStringLiteral("if (panel.requiresExplicitStatus && statusField.currentIndex < 0) return")));
    QVERIFY(editorSource.contains(QStringLiteral("panel.close()")));
}

void QmlStructureTests::mainKeepsSeasonalCatalogAsSeparateNavigation() {
    const QString source = qmlSource(QStringLiteral("Main.qml"));
    QVERIFY(source.contains(QStringLiteral("property string page: \"home\"")));
    QVERIFY(source.contains(QStringLiteral("SeasonalCatalogScreen {")));
    QVERIFY(source.contains(QStringLiteral("onOpenSeasonalCatalogRequested: root.page = \"seasonal\"")));
    QVERIFY(source.contains(QStringLiteral("onBackRequested: root.page = \"home\"")));
}

QTEST_GUILESS_MAIN(QmlStructureTests)
#include "QmlStructureTests.moc"
