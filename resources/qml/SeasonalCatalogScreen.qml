import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: seasonalCatalog

    signal backRequested()

    property var controller: seasonalCatalogController
    readonly property color ink: "#172033"
    readonly property color muted: "#68758a"
    readonly property color line: "#d7dee8"
    readonly property color surface: "#ffffff"
    readonly property color surfaceSoft: "#f6f8fb"
    readonly property color accent: "#315d91"

    function indexForKey(options, key) {
        for (let index = 0; index < options.length; ++index) {
            if (options[index].key === key) return index
        }
        return -1
    }

    function updateContentIndex() {
        const nextIndex = controller.state === "loading"
                ? (controller.hasResults ? 4 : 0)
                : controller.state === "empty" ? 1
                : controller.state === "error" ? 2
                : controller.state === "populated" ? 4 : 3
        if (catalogContent.currentIndex !== nextIndex) catalogContent.currentIndex = nextIndex
    }

    Connections {
        target: controller
        function onStateChanged() { seasonalCatalog.updateContentIndex() }
    }

    Component.onCompleted: updateContentIndex()

    Rectangle { anchors.fill: parent; color: "#eef2f7" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 18

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 82
            spacing: 14

            ColumnLayout {
                id: seasonalHeaderTitle
                Layout.fillWidth: true
                spacing: 2

                Label {
                    text: qsTr("HAIKEN ANIME")
                    color: accent
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    font.letterSpacing: 2.2
                }

                Label {
                    text: qsTr("Catálogo sazonal")
                    color: ink
                    font.pixelSize: 25
                    font.weight: Font.Bold
                }

                Button {
                    id: seasonalHeaderNavigation
                    text: qsTr("‹  Biblioteca")
                    flat: true
                    font.pixelSize: 13
                    onClicked: seasonalCatalog.backRequested()
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: line }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: filterLayout.implicitHeight + 32
            color: surface
            border.color: line
            border.width: 1
            radius: 8

            Flow {
                id: filterLayout
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Label { text: qsTr("Filtrar por"); color: muted; font.pixelSize: 12 }

                ComboBox {
                    id: yearSelector
                    width: filterLayout.width >= 560 ? 150 : filterLayout.width
                    model: controller.availableYearOptions
                    textRole: "label"
                    valueRole: "key"
                    currentIndex: controller.selectedYear > 0
                                  ? seasonalCatalog.indexForKey(controller.availableYearOptions,
                                                                String(controller.selectedYear)) : -1
                    displayText: currentIndex < 0 ? qsTr("Ano") : currentText
                    onActivated: controller.SetYear(Number(currentValue))
                }

                ComboBox {
                    id: seasonSelector
                    width: filterLayout.width >= 560 ? 170 : filterLayout.width
                    model: controller.availableSeasonOptions
                    textRole: "label"
                    valueRole: "key"
                    currentIndex: controller.selectedSeasonKey.length > 0
                                  ? seasonalCatalog.indexForKey(controller.availableSeasonOptions,
                                                                controller.selectedSeasonKey) : -1
                    displayText: currentIndex < 0 ? qsTr("Temporada") : currentText
                    onActivated: controller.SetSeason(currentValue)
                }

                Button {
                    text: qsTr("Tentar novamente")
                    visible: controller.state === "error"
                    onClicked: controller.Retry()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: surface
            border.color: line
            border.width: 1
            radius: 8

            StackLayout {
                id: catalogContent
                anchors.fill: parent
                anchors.margins: 18

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        id: initialLoadingState
                        anchors.centerIn: parent
                        spacing: 10

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Carregando catálogo…")
                            color: ink
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                        }

                        BusyIndicator {
                            Layout.alignment: Qt.AlignHCenter
                            running: true
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        id: emptyState
                        anchors.centerIn: parent
                        spacing: 10

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Nenhum título encontrado")
                            color: ink
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.maximumWidth: 480
                            text: qsTr("Ajuste os filtros ou tente novamente.")
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            color: muted
                        }

                        Button {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Tentar novamente")
                            onClicked: controller.Retry()
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        id: errorState
                        anchors.centerIn: parent
                        spacing: 10

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Não foi possível carregar o catálogo")
                            color: "#8f3038"
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.maximumWidth: 480
                            text: controller.errorMessage
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            color: muted
                        }

                        Button {
                            id: seasonalErrorRetryButton
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Tentar novamente")
                            onClicked: controller.Retry()
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 10

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Escolha um ano e uma temporada")
                            color: ink
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.maximumWidth: 480
                            text: qsTr("Os resultados só são buscados quando ambos os filtros forem selecionados.")
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            color: muted
                        }
                    }
                }

                GridView {
                    id: resultsGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    readonly property int columns: width >= 860 ? 3 : width >= 560 ? 2 : 1
                    cellWidth: width / columns
                    cellHeight: 154
                    model: controller.mediaModel

                    function requestNextPageIfNearEnd() {
                        if (!controller.canLoadNextPage) return
                        const remainingContent = contentHeight - (contentY + height)
                        if (remainingContent <= cellHeight * columns) controller.LoadNextPage()
                    }

                    onContentYChanged: requestNextPageIfNearEnd()
                    onContentHeightChanged: requestNextPageIfNearEnd()
                    onHeightChanged: requestNextPageIfNearEnd()
                    onVisibleChanged: requestNextPageIfNearEnd()
                    delegate: MediaCard {
                        width: resultsGrid.cellWidth - 12
                        height: 142
                        mediaId: model.mediaId
                        title: model.title
                        status: model.statusLabel
                        progress: model.progress
                        score: model.score
                        coverSource: model.coverSource.length > 0
                                     ? model.coverSource
                                     : "qrc:/qt/qml/HaikenAnime/resources/images/cover-placeholder.svg"
                        selected: controller.selectedMediaId === model.mediaId
                        onActivated: function(mediaId) {
                            controller.SelectMedia(mediaId)
                            seasonalDetails.openForItem(this)
                        }
                    }

                    footer: Item {
                        width: resultsGrid.width
                        height: controller.canLoadNextPage
                                || (controller.state === "loading" && controller.hasResults) ? 62 : 0
                        BusyIndicator {
                            id: nextPageLoadingIndicator
                            anchors.centerIn: parent
                            visible: controller.state === "loading" && controller.hasResults
                            running: visible
                        }
                    }
                }
            }
        }
    }

    MediaDetailsPanel {
        id: seasonalDetails
        controller: seasonalCatalog.controller
        seasonalLayout: true
        ink: seasonalCatalog.ink
        muted: seasonalCatalog.muted
        line: seasonalCatalog.line
        surface: seasonalCatalog.surface
        surfaceSoft: seasonalCatalog.surfaceSoft
        accent: seasonalCatalog.accent
        seasonalListErrorMessage: controller.personalListErrorMessage
        onAddToMyListRequested: {
            personalListEditor.openForMedia(controller.selectedMediaId,
                                            controller.selectedProgressValue,
                                            "", controller.selectedScoreValue,
                                            "", controller.selectedAlternativeNames.join("; "))
        }
        onEditPersonalListRequested: {
            personalListEditor.openForMedia(controller.selectedMediaId,
                                            controller.selectedProgressValue,
                                            controller.selectedListStatusKey,
                                            controller.selectedScoreValue,
                                            controller.selectedLocalPath,
                                            controller.selectedAlternativeNames.join("; "))
        }
    }

    EditMediaPanel {
        id: personalListEditor
        controller: seasonalCatalog.controller
        listOptions: seasonalCatalog.controller.availablePersonalListOptions
        requiresExplicitStatus: true
        closeOnApply: false
        ink: seasonalCatalog.ink
        muted: seasonalCatalog.muted
        line: seasonalCatalog.line
        surface: seasonalCatalog.surface
        surfaceSoft: seasonalCatalog.surfaceSoft
        accent: seasonalCatalog.accent
        onApplyRequested: function(mediaId, progress, statusKey, score, path, alternativeNames) {
            if (controller.SaveSelectedToPersonalListFromEditor(progress, statusKey, score, path, alternativeNames)) personalListEditor.close()
        }
    }
}
