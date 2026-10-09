import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts 1.15

Item {
    id: home

    signal openSettingsRequested()
    signal openSeasonalCatalogRequested()

    onVisibleChanged: if (visible) {
        previewBrowseControls.synchronizeWithController()
        completeBrowseControls.synchronizeWithController()
        mediaGrid.reportWindow()
    }

    property var controller: homeController

    readonly property color ink: "#172033"
    readonly property color muted: "#68758a"
    readonly property color line: "#d7dee8"
    readonly property color surface: "#ffffff"
    readonly property color surfaceSoft: "#f6f8fb"
    readonly property color accent: "#315d91"
    readonly property color accentSoft: "#e2ebf6"
    readonly property bool hasLocalLibraryMessage: controller.hasSelection
                                                  && (controller.localLibraryStatusMessage.length > 0
                                                      || controller.localLibraryErrorMessage.length > 0)

    function clearBrowseControlFocus() {
        previewBrowseControls.clearControlFocus()
        completeBrowseControls.clearControlFocus()
        home.forceActiveFocus()
    }

    TapHandler {
        gesturePolicy: TapHandler.DragThreshold
        onTapped: home.clearBrowseControlFocus()
    }

    function openEditor() {
        editMediaPanel.openForMedia(
                    controller.selectedMediaId,
                    controller.selectedProgressValue,
                    controller.selectedListStatusKey,
                    controller.selectedScoreValue,
                    "",
                    controller.selectedAlternativeNames.join("; "))
    }

    Rectangle {
        anchors.fill: parent
        color: "#eef2f7"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 18

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 82
            spacing: 18

            ColumnLayout {
                id: homeHeaderTitle
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
                    text: qsTr("Sua biblioteca")
                    color: ink
                    font.pixelSize: 25
                    font.weight: Font.Bold
                }

                Button {
                    id: homeHeaderNavigationSpacer
                    text: qsTr("‹  Biblioteca")
                    opacity: 0
                    enabled: false
                    focus: false
                    Layout.preferredHeight: implicitHeight
                }
            }

            Button {
                text: qsTr("Catálogo sazonal")
                flat: true
                font.pixelSize: 13
                Layout.alignment: Qt.AlignVCenter
                onClicked: home.openSeasonalCatalogRequested()
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 28
                color: line
            }

            Button {
                text: qsTr("Configurações")
                flat: true
                font.pixelSize: 13
                Layout.alignment: Qt.AlignVCenter
                onClicked: home.openSettingsRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: line
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            Rectangle {
                id: libraryPreviewPanel
                Layout.fillWidth: true
                Layout.minimumWidth: 680
                Layout.horizontalStretchFactor: 3
                Layout.fillHeight: true
                color: surface
                radius: 8
                border.color: line
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 14

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                text: qsTr("Minha lista")
                                color: ink
                                font.pixelSize: 18
                                font.weight: Font.DemiBold
                            }

                            Label {
                                text: qsTr("%1 títulos nesta categoria").arg(controller.filteredMediaCount)
                                color: muted
                                font.pixelSize: 12
                            }
                        }

                        Row {
                            spacing: 4

                            Repeater {
                                model: controller.availableMediaTypeOptions

                                delegate: Rectangle {
                                    required property var modelData
                                    readonly property bool selectedType: controller.activeMediaType === modelData.key
                                    width: typeLabel.implicitWidth + 24
                                    height: 32
                                    radius: 5
                                    color: selectedType ? accentSoft : "transparent"
                                    border.color: selectedType || activeFocus ? accent : line
                                    border.width: activeFocus ? 2 : 1
                                    activeFocusOnTab: true
                                    Accessible.role: Accessible.Button
                                    Accessible.name: modelData.label
                                    Accessible.onPressAction: controller.SetMediaType(modelData.key)

                                    Label {
                                        id: typeLabel
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        color: parent.selectedType ? accent : muted
                                        font.pixelSize: 11
                                        font.weight: parent.selectedType ? Font.DemiBold : Font.Normal
                                    }

                                    TapHandler {
                                        onTapped: controller.SetMediaType(modelData.key)
                                    }
                                    Keys.onReturnPressed: controller.SetMediaType(modelData.key)
                                    Keys.onSpacePressed: controller.SetMediaType(modelData.key)
                                }
                            }
                        }

                        Button {
                            text: qsTr("Ver lista completa")
                            enabled: controller.filteredMediaCount > 0
                            flat: true
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            onClicked: completeLibrary.open()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: line
                    }

                    GridView {
                        id: mediaGrid
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        readonly property int columns: width >= 760 ? 3 : 2
                        readonly property int previewRows: 3
                        cellWidth: width / columns
                        cellHeight: Math.floor(height / previewRows)
                        cacheBuffer: height
                        visible: controller.filteredMediaCount > 0
                        model: controller.mediaModel
                        delegate: MediaCard {
                            width: mediaGrid.cellWidth - 12
                            height: mediaGrid.cellHeight - 12
                            mediaId: model.mediaId
                            coverSource: model.coverSource
                            title: model.title
                            status: model.statusLabel
                            progress: model.progress
                            score: model.score
                            selected: controller.selectedMediaId === model.mediaId
                            muted: false
                            onActivated: function(mediaId) {
                                controller.SelectMedia(mediaId)
                            }
                            onCoverLoadFailed: function(mediaId) {
                                home.controller.ReportCoverLoadFailure(mediaId)
                            }
                        }
                        function reportWindow() {
                            if (count <= 0 || height <= 0) return
                            const first = Math.max(0, indexAt(1, contentY + 1))
                            let last = indexAt(width - 2, contentY + height - 2)
                            if (last < first) last = Math.min(count - 1, first + columns * Math.ceil(height / cellHeight))
                            controller.RequestCoverWindow("preview", first, last, columns * 2)
                        }
                        onContentYChanged: windowTimer.restart()
                        onHeightChanged: windowTimer.restart()
                        onWidthChanged: windowTimer.restart()
                        onCountChanged: windowTimer.restart()
                        Timer { id: windowTimer; interval: 80; repeat: false; onTriggered: mediaGrid.reportWindow() }
                    }

                    StatePanel {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: controller.filteredMediaCount === 0 && controller.state === "ready"
                        title: qsTr("Nenhum título encontrado")
                        description: controller.browseCriteriaActive
                                     ? qsTr("Ajuste a lista selecionada ou o texto da busca para ver outros resultados.")
                                     : qsTr("Escolha outro tipo de mídia para continuar navegando pela sua biblioteca.")
                        stateLabel: controller.activeMediaType === "anime" ? qsTr("ANIME")
                                    : controller.activeMediaType === "manga" ? qsTr("MANGA") : qsTr("NOVEL")
                        accentColor: accent
                    }

                    Button {
                        Layout.alignment: Qt.AlignHCenter
                        visible: controller.filteredMediaCount === 0 && controller.browseCriteriaActive
                        text: qsTr("Limpar filtros")
                        flat: true
                        onClicked: controller.ClearBrowseCriteria()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.minimumWidth: 400
                Layout.horizontalStretchFactor: 2
                Layout.fillHeight: true
                color: surface
                radius: 8
                border.color: line
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 26
                    spacing: 18

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("DETALHES")
                            color: accent
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.5
                        }
                        Button {
                            id: detailsButton
                            text: qsTr("Ver detalhes")
                            flat: true
                            visible: controller.hasSelection
                            enabled: controller.hasSelection
                            onClicked: mediaDetailsPanel.openForItem(detailsButton)
                        }
                        Button {
                            text: qsTr("Editar")
                            flat: true
                            visible: controller.hasSelection
                            enabled: controller.hasSelection
                            onClicked: home.openEditor()
                        }
                    }

                    StatePanel {
                        visible: !controller.hasSelection
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: qsTr("Nenhum título selecionado")
                        description: controller.state === "error"
                                     ? controller.statusMessage
                                     : qsTr("Os detalhes da mídia selecionada aparecerão aqui quando sua biblioteca tiver dados.")
                        stateLabel: controller.state === "loading"
                                    ? qsTr("CARREGANDO")
                                    : controller.state === "error"
                                      ? qsTr("ERRO")
                                      : controller.state === "ready"
                                        ? qsTr("DADOS DISPONÍVEIS")
                                        : qsTr("ESTADO VAZIO")
                        accentColor: accent
                    }

                    ColumnLayout {
                        visible: controller.hasSelection
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 14

                        Rectangle {
                            id: coverTrigger
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 150
                            Layout.preferredHeight: 214
                            radius: 6
                            color: accentSoft
                            clip: true
                            activeFocusOnTab: true
                            Accessible.role: Accessible.Button
                            Accessible.name: qsTr("Ampliar capa selecionada")

                            TapHandler {
                                onTapped: coverPreview.openForSource(coverTrigger)
                            }
                            Keys.onReturnPressed: coverPreview.openForSource(coverTrigger)
                            Keys.onSpacePressed: coverPreview.openForSource(coverTrigger)

                            Image {
                                anchors.fill: parent
                                source: controller.selectedCoverSource
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                cache: false
                                onStatusChanged: if (status === Image.Error)
                                                     controller.ReportCoverLoadFailure(controller.selectedMediaId)
                            }
                        }

                        TextEdit {
                            id: selectedTitleText
                            Layout.fillWidth: true
                            Layout.preferredHeight: contentHeight
                            text: controller.selectedTitle
                            color: ink
                            font.pixelSize: 20
                            font.weight: Font.Bold
                            readOnly: true
                            selectByMouse: true
                            wrapMode: TextEdit.Wrap
                            Accessible.name: qsTr("Título selecionado")
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            TextEdit {
                                id: selectedTypeText
                                Layout.preferredWidth: contentWidth
                                Layout.preferredHeight: contentHeight
                                text: controller.selectedTypeLabel
                                color: accent
                                font.weight: Font.DemiBold
                                readOnly: true
                                selectByMouse: true
                                wrapMode: TextEdit.NoWrap
                                Accessible.name: qsTr("Tipo da mídia selecionada")
                            }
                            Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 16; color: line }
                            TextEdit {
                                id: selectedStatusText
                                Layout.preferredHeight: contentHeight
                                text: controller.selectedListStatusLabel
                                color: muted
                                readOnly: true
                                selectByMouse: true
                                wrapMode: TextEdit.NoWrap
                                Accessible.name: qsTr("Status da mídia selecionada")
                            }
                            Rectangle { visible: controller.availableLocalEpisodeCount > 0; Layout.preferredWidth: 1; Layout.preferredHeight: 16; color: line }
                            TextEdit { visible: controller.availableLocalEpisodeCount > 0; Layout.preferredHeight: contentHeight; text: qsTr("%1 episódios disponíveis localmente").arg(controller.availableLocalEpisodeCount); color: muted; readOnly: true; selectByMouse: true; wrapMode: TextEdit.NoWrap }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 18
                            rowSpacing: 4

                            Label { text: qsTr("Progresso"); color: muted; font.pixelSize: 11 }
                            Label { text: qsTr("Sua nota"); color: muted; font.pixelSize: 11 }
                            TextEdit {
                                id: selectedProgressText
                                Layout.fillWidth: true
                                Layout.preferredHeight: contentHeight
                                text: controller.selectedProgress
                                color: ink
                                font.weight: Font.DemiBold
                                readOnly: true
                                selectByMouse: true
                                wrapMode: TextEdit.NoWrap
                                Accessible.name: qsTr("Progresso da mídia selecionada")
                            }
                            TextEdit {
                                id: selectedScoreText
                                Layout.fillWidth: true
                                Layout.preferredHeight: contentHeight
                                text: controller.selectedScore
                                color: ink
                                font.weight: Font.DemiBold
                                readOnly: true
                                selectByMouse: true
                                wrapMode: TextEdit.NoWrap
                                Accessible.name: qsTr("Nota da mídia selecionada")
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            TextEdit {
                                id: selectedSynopsisText
                                anchors.fill: parent
                                text: controller.selectedSynopsis.length > 0
                                      ? controller.selectedSynopsis
                                      : qsTr("Sinopse não disponível.")
                                color: muted
                                font.pixelSize: 12
                                readOnly: true
                                selectByMouse: true
                                wrapMode: TextEdit.Wrap
                                verticalAlignment: TextEdit.AlignTop
                                clip: true
                                Accessible.name: qsTr("Sinopse da mídia selecionada")
                            }

                            Text {
                                id: selectedSynopsisOverflowIndicator
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                color: muted
                                enabled: false
                                text: "…"
                                visible: selectedSynopsisText.contentHeight > selectedSynopsisText.height
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: line
                    }

                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: controller.state === "error" ? 112
                                  : controller.state === "loading" ? 102 : 72
            color: controller.state === "error" ? "#fff0f0" : surfaceSoft
            radius: 8
            border.color: controller.state === "error" ? "#e6a6aa" : line
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 7

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    BrowseControls {
                        id: previewBrowseControls
                        Layout.preferredWidth: libraryPreviewPanel.width - 16
                        Layout.maximumWidth: libraryPreviewPanel.width - 16
                        controller: home.controller
                        ink: home.ink
                        muted: home.muted
                        line: home.line
                        surface: home.surface
                        accent: home.accent
                    }

                    Basic.Button {
                        id: watchNextButton
                        Layout.leftMargin: 8
                        Layout.preferredWidth: 104
                        Layout.preferredHeight: 44
                        text: qsTr("Assistir")
                        enabled: controller.canWatch
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        Accessible.name: qsTr("Assistir próximo episódio disponível")
                        onClicked: controller.WatchNext()

                        contentItem: Text {
                            text: watchNextButton.text
                            color: watchNextButton.enabled ? "#ffffff" : muted
                            font: watchNextButton.font
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            radius: 5
                            color: watchNextButton.enabled ? accent : surface
                            border.width: watchNextButton.activeFocus ? 2 : 1
                            border.color: watchNextButton.enabled
                                          ? accent
                                          : (watchNextButton.activeFocus ? accent : line)
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 1
                        Layout.preferredHeight: 28
                        color: line
                    }

                    ColumnLayout {
                        id: homeSynchronizationStatus
                        Layout.preferredWidth: 210
                        spacing: 1

                        Label {
                            text: controller.state === "error" ? qsTr("ERRO")
                                  : home.hasLocalLibraryMessage ? qsTr("BIBLIOTECA LOCAL")
                                  : qsTr("SINCRONIZAÇÃO")
                            color: controller.state === "error" || controller.localLibraryErrorMessage.length > 0
                                   ? "#b13b43" : accent
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.0
                        }

                        Label {
                            id: homeSynchronizationDetail
                            Layout.fillWidth: true
                            text: controller.state === "error"
                                  ? (controller.errorMessage.length > 0
                                     ? controller.errorMessage
                                     : qsTr("Verifique os detalhes"))
                                  : home.hasLocalLibraryMessage
                                    ? (controller.localLibraryErrorMessage.length > 0
                                       ? controller.localLibraryErrorMessage
                                       : controller.localLibraryStatusMessage)
                                    : controller.state === "loading"
                                      ? (controller.synchronizationProgressKnown
                                         ? qsTr("Atualizando · %1%").arg(controller.synchronizationProgress)
                                         : qsTr("Atualizando biblioteca"))
                                      : qsTr("Sincronização automática")
                            color: controller.state === "error" || controller.localLibraryErrorMessage.length > 0
                                   ? "#b13b43" : ink
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            wrapMode: Text.Wrap
                            maximumLineCount: controller.state === "error" ? 2 : 1
                            elide: Text.ElideRight
                        }
                    }
                }

                ProgressBar {
                    Layout.fillWidth: true
                    visible: controller.state === "loading"
                    from: 0
                    to: 100
                    value: controller.synchronizationProgress
                    indeterminate: !controller.synchronizationProgressKnown
                }
            }
        }
    }

    EditMediaPanel {
        id: editMediaPanel
        z: completeLibrary.z + 1
        controller: home.controller
        listOptions: home.controller.availableListOptions
        closeOnApply: false
        ink: home.ink
        muted: home.muted
        line: home.line
        surface: home.surface
        surfaceSoft: home.surfaceSoft
        accent: home.accent
        onApplyRequested: function(mediaId, progress, statusKey, score, path, alternativeNames) {
            const names = alternativeNames.split(";").map(function(name) { return name.trim() })
                                          .filter(function(name) { return name.length > 0 })
            if (home.controller.SaveSelectedMediaFromEditor(progress, statusKey, score, path, names)) {
                editMediaPanel.close()
            }
        }
    }

    CoverPreview {
        id: coverPreview
        source: controller.selectedCoverSource
        ink: home.ink
        muted: home.muted
        line: home.line
        surface: home.surface
    }

    MediaDetailsPanel {
        id: mediaDetailsPanel
        z: completeLibrary.z + 1
        controller: home.controller
        ink: home.ink
        muted: home.muted
        line: home.line
        surface: home.surface
        surfaceSoft: home.surfaceSoft
        accent: home.accent
    }

    Popup {
        id: completeLibrary
        z: 1
        parent: Overlay.overlay
        x: 0
        y: 0
        width: Math.min(parent.width, Math.max(720, Math.round(parent.width * 0.605)))
        height: parent.height
        modal: true
        dim: false
        focus: true
        padding: 0
        closePolicy: Popup.CloseOnEscape
        onOpened: completeGrid.reportWindow()
        onClosed: mediaGrid.reportWindow()

        background: Rectangle {
            color: surface
            border.color: line
            border.width: 1
        }

        contentItem: ColumnLayout {
            spacing: 0

            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 22
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Lista completa")
                        color: ink
                        font.pixelSize: 20
                        font.weight: Font.Bold
                    }

                    Button {
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                        text: qsTr("Fechar")
                        flat: true
                        onClicked: completeLibrary.close()
                    }
                }

                Label {
                    text: qsTr("%1 títulos · %2").arg(controller.filteredMediaCount)
                          .arg(controller.activeMediaType === "anime" ? qsTr("Anime")
                               : controller.activeMediaType === "manga" ? qsTr("Manga") : qsTr("Novel"))
                    color: muted
                    font.pixelSize: 12
                }

                BrowseControls {
                    id: completeBrowseControls
                    Layout.fillWidth: true
                    controller: home.controller
                    ink: home.ink
                    muted: home.muted
                    line: home.line
                    surface: home.surface
                    accent: home.accent
                }
            }

            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: line }

            GridView {
                id: completeGrid
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 18
                clip: true
                readonly property int columns: width >= 820 ? 4 : 3
                cellWidth: width / columns
                cellHeight: 154
                cacheBuffer: height
                model: controller.fullMediaModel
                delegate: MediaCard {
                    width: completeGrid.cellWidth - 12
                    height: 142
                    compactMetadata: true
                    mediaId: model.mediaId
                    coverSource: model.coverSource
                    title: model.title
                    status: model.statusLabel
                    progress: model.progress
                    score: model.score
                    selected: controller.selectedMediaId === model.mediaId
                    onActivated: function(mediaId) {
                        controller.SelectMedia(mediaId)
                    }
                    onCoverLoadFailed: function(mediaId) {
                        home.controller.ReportCoverLoadFailure(mediaId)
                    }
                }

                function reportWindow() {
                    if (!completeLibrary.opened || count <= 0 || height <= 0) return
                    const first = Math.max(0, indexAt(1, contentY + 1))
                    let last = indexAt(width - 2, contentY + height - 2)
                    if (last < first) last = Math.min(count - 1,
                                                      first + columns * Math.ceil(height / cellHeight))
                    controller.RequestCoverWindow("full", first, last, columns * 2)
                }
                onContentYChanged: completeWindowTimer.restart()
                onHeightChanged: completeWindowTimer.restart()
                onWidthChanged: completeWindowTimer.restart()
                onCountChanged: completeWindowTimer.restart()
                Timer {
                    id: completeWindowTimer
                    interval: 80
                    repeat: false
                    onTriggered: completeGrid.reportWindow()
                }
            }
        }
    }

}
