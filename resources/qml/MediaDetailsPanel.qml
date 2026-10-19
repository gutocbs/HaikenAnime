import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: panel

    property var controller
    property Item returnFocusItem: null
    readonly property Theme theme: Theme {}
    property bool seasonalLayout: false
    property string seasonalListErrorMessage: ""

    signal addToMyListRequested()
    signal editPersonalListRequested()

    function openForItem(focusItem) {
        returnFocusItem = focusItem
        open()
    }

    parent: Overlay.overlay
    x: 0
    y: 0
    width: parent ? Math.min(parent.width, Math.max(320, Math.round(parent.width * 0.48))) : 480
    height: parent ? parent.height : 720
    modal: true
    dim: true
    focus: true
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    onOpened: closeButton.forceActiveFocus()
    onClosed: {
        if (returnFocusItem) returnFocusItem.forceActiveFocus()
    }
    Overlay.modal: Rectangle { color: "#730f1d2e" }
    background: Rectangle {
        color: theme.surface
        border.color: theme.line
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 0

        Keys.priority: Keys.BeforeItem
        Keys.onTabPressed: function(event) {
            closeButton.forceActiveFocus()
            event.accepted = true
        }
        Keys.onBacktabPressed: function(event) {
            closeButton.forceActiveFocus()
            event.accepted = true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 20

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    text: qsTr("DETALHES COMPLETOS")
                    color: theme.accent
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.5
                }

                Label {
                    Layout.fillWidth: true
                    text: controller.selectedTitle
                    color: theme.ink
                    font.pixelSize: 20
                    font.weight: Font.Bold
                    wrapMode: Text.Wrap
                }
            }

            AppButton {
                id: closeButton
                text: qsTr("Fechar")
                variant: "flat"
                onClicked: panel.close()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: theme.line
        }

        ScrollView {
            id: detailsScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 20
            clip: true
            contentWidth: availableWidth

            RowLayout {
                id: seasonalDetailsLayout
                width: detailsScroll.availableWidth
                spacing: 18

                ColumnLayout {
                    id: seasonalMetadata
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: 18

                    GridLayout {
                        Layout.fillWidth: true
                        columns: width >= 400 ? 2 : 1
                        columnSpacing: 18
                        rowSpacing: 10

                        Label { text: qsTr("Tipo"); color: theme.muted; font.pixelSize: 11 }
                        Label { text: controller.selectedTypeLabel; color: theme.ink; font.weight: Font.DemiBold }
                        Label { text: qsTr("Status"); color: theme.muted; font.pixelSize: 11 }
                        Label { text: controller.selectedStatusLabel; color: theme.ink; font.weight: Font.DemiBold }
                        Label { text: qsTr("Progresso"); color: theme.muted; font.pixelSize: 11 }
                        Label { text: controller.selectedProgress; color: theme.ink; font.weight: Font.DemiBold }
                        Label { text: qsTr("Sua nota"); color: theme.muted; font.pixelSize: 11 }
                        Label { text: controller.selectedScore; color: theme.ink; font.weight: Font.DemiBold }
                        Label { text: qsTr("Nota AniList"); color: theme.muted; font.pixelSize: 11 }
                        Label { text: controller.selectedAverageScore; color: theme.ink; font.weight: Font.DemiBold }
                        Label {
                            visible: controller.selectedSeasonLabel.length > 0
                            text: qsTr("Temporada")
                            color: theme.muted
                            font.pixelSize: 11
                        }
                        Label {
                            visible: controller.selectedSeasonLabel.length > 0
                            text: controller.selectedSeasonLabel
                            color: theme.ink
                            font.weight: Font.DemiBold
                        }
                        Label {
                            visible: controller.selectedNextAiringLabel.length > 0
                            text: qsTr("Próximo episódio")
                            color: theme.muted
                            font.pixelSize: 11
                        }
                        Label {
                            visible: controller.selectedNextAiringLabel.length > 0
                            text: controller.selectedNextAiringLabel
                            color: theme.ink
                            font.weight: Font.DemiBold
                            wrapMode: Text.Wrap
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: controller.selectedAlternativeNames.length > 0
                        spacing: 5

                        Label {
                            text: qsTr("Também conhecido como")
                            color: theme.muted
                            font.pixelSize: 11
                        }

                        TextEdit {
                            Layout.fillWidth: true
                            Layout.preferredHeight: contentHeight
                            text: controller.selectedAlternativeNames.join(" · ")
                            color: theme.ink
                            readOnly: true
                            selectByMouse: true
                            wrapMode: TextEdit.Wrap
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5

                        Label {
                            text: qsTr("Sinopse")
                            color: theme.muted
                            font.pixelSize: 11
                        }

                        TextEdit {
                            id: fullSynopsisText
                            visible: controller.selectedSynopsis.length > 0
                            Layout.fillWidth: true
                            Layout.preferredHeight: contentHeight
                            text: controller.selectedSynopsis
                            color: theme.ink
                            readOnly: true
                            selectByMouse: true
                            wrapMode: TextEdit.Wrap
                            Accessible.name: qsTr("Sinopse completa")
                        }

                        Label {
                            visible: controller.selectedSynopsis.length === 0
                            text: qsTr("Sinopse não disponível.")
                            color: theme.muted
                            font.pixelSize: 13
                        }
                    }

                    ColumnLayout {
                        // "Assistindo agora" passes a plain map without the folder field.
                        readonly property string localFolderUrl: controller.selectedLocalFolderUrl || ""
                        id: linksSection
                        Layout.fillWidth: true
                        visible: controller.selectedMediaLinks.length > 0 || localFolderUrl.length > 0
                        spacing: 5

                        Label {
                            text: qsTr("Links")
                            color: theme.muted
                            font.pixelSize: 11
                        }

                        GridLayout {
                            id: externalLinksGrid
                            Layout.fillWidth: true
                            columns: width >= 340 ? 2 : 1
                            columnSpacing: 8
                            rowSpacing: 4

                            Repeater {
                                model: controller.selectedMediaLinks

                                delegate: AppButton {
                                    id: externalLinkButton
                                    required property var modelData
                                    readonly property color serviceColor: {
                                        const site = (modelData.site || "").toLowerCase()
                                        if (site.indexOf("crunchyroll") >= 0) return "#f47521"
                                        if (site.indexOf("netflix") >= 0) return "#b20710"
                                        if (site.indexOf("amazon") >= 0) return "#00a8e1"
                                        if (site.indexOf("anilist") >= 0) return "#02a9ff"
                                        return theme.accent
                                    }
                                    Layout.fillWidth: true
                                    text: modelData.site
                                    baseColor: externalLinkButton.serviceColor
                                    Accessible.name: qsTr("Abrir %1").arg(modelData.site)
                                    onClicked: Qt.openUrlExternally(modelData.url)
                                }
                            }

                            AppButton {
                                visible: linksSection.localFolderUrl.length > 0
                                Layout.fillWidth: true
                                text: qsTr("Abrir pasta")
                                variant: "secondary"
                                Accessible.name: qsTr("Abrir a pasta local")
                                onClicked: Qt.openUrlExternally(linksSection.localFolderUrl)
                            }
                        }
                    }
                }

                Image {
                    id: seasonalCover
                    visible: panel.seasonalLayout
                    Layout.alignment: Qt.AlignTop
                    readonly property real coverWidth: Math.min(180, Math.max(96,
                        Math.round(detailsScroll.availableWidth * 0.32)))
                    Layout.preferredWidth: coverWidth
                    Layout.preferredHeight: Math.round(coverWidth * 1.45)
                    source: controller.selectedCoverSource
                    fillMode: Image.PreserveAspectCrop
                    clip: true
                }
            }
        }

        Rectangle {
            visible: panel.seasonalLayout
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? 1 : 0
            color: theme.line
        }

        RowLayout {
            visible: panel.seasonalLayout
            Layout.fillWidth: true
            Layout.margins: 18
            spacing: 12

            Label {
                Layout.fillWidth: true
                visible: panel.seasonalListErrorMessage.length > 0
                text: panel.seasonalListErrorMessage
                color: "#8f3038"
                font.pixelSize: 11
                wrapMode: Text.Wrap
            }

            Item { Layout.fillWidth: panel.seasonalListErrorMessage.length === 0 }

            AppButton {
                id: addToMyListButton
                visible: panel.seasonalLayout && !controller.selectedMediaInPersonalList
                text: qsTr("Adicionar à minha lista")
                onClicked: panel.addToMyListRequested()
            }

            AppButton {
                id: editPersonalListButton
                visible: panel.seasonalLayout && controller.selectedMediaInPersonalList
                text: qsTr("Editar mídia")
                onClicked: panel.editPersonalListRequested()
            }
        }
    }
}
