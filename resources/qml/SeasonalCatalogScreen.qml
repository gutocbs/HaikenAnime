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

    Rectangle { anchors.fill: parent; color: "#eef2f7" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 18

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Button {
                text: qsTr("Voltar")
                flat: true
                onClicked: seasonalCatalog.backRequested()
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    text: qsTr("CATÁLOGO")
                    color: accent
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    font.letterSpacing: 2.2
                }

                Label {
                    text: qsTr("Temporada")
                    color: ink
                    font.pixelSize: 25
                    font.weight: Font.Bold
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
                anchors.fill: parent
                anchors.margins: 18
                currentIndex: controller.state === "populated" ? 1 : 0

                ColumnLayout {
                    Layout.alignment: Qt.AlignCenter
                    spacing: 10

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: controller.state === "loading" ? qsTr("Carregando catálogo…")
                              : controller.state === "error" ? qsTr("Não foi possível carregar o catálogo")
                              : controller.state === "empty" ? qsTr("Nenhum título encontrado")
                              : qsTr("Escolha um ano e uma temporada")
                        color: controller.state === "error" ? "#8f3038" : ink
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.maximumWidth: 480
                        text: controller.state === "error" ? controller.errorMessage
                              : controller.state === "empty" ? qsTr("Ajuste os filtros ou tente novamente.")
                              : qsTr("Os resultados só são buscados quando ambos os filtros forem selecionados.")
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        color: muted
                    }

                    BusyIndicator { Layout.alignment: Qt.AlignHCenter; visible: controller.state === "loading"; running: visible }

                    Button {
                        Layout.alignment: Qt.AlignHCenter
                        visible: controller.state === "error" || controller.state === "empty"
                        text: qsTr("Tentar novamente")
                        onClicked: controller.Retry()
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
                        height: controller.canLoadNextPage ? 62 : 0
                        Button {
                            anchors.centerIn: parent
                            visible: controller.canLoadNextPage
                            text: qsTr("Carregar próxima página")
                            onClicked: controller.LoadNextPage()
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
    }
}
