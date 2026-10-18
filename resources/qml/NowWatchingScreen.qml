import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// "Assistindo agora", laid out like the Home: open sessions on the left (one card per player
// instance), details of the selected one on the right. Anime data comes from the library; episode
// data is only what the player reported. Read-only: the controller pushes every change.
Item {
    id: nowWatching

    signal backRequested()

    property var controller: nowWatchingController
    readonly property var current: controller ? controller.current : ({})
    readonly property var openSessions: controller ? controller.sessions : []
    readonly property bool hasSession: controller !== null && controller.active

    readonly property color ink: "#172033"
    readonly property color muted: "#68758a"
    readonly property color line: "#d7dee8"
    readonly property color surface: "#ffffff"
    readonly property color surfaceSoft: "#f6f8fb"
    readonly property color accent: "#315d91"
    readonly property color accentSoft: "#e2ebf6"
    readonly property string placeholderCover: "qrc:/qt/qml/HaikenAnime/resources/images/cover-placeholder.svg"

    function coverOf(item) {
        return item.coverUrl !== undefined ? item.coverUrl : placeholderCover
    }

    function titleOf(item) {
        return item.animeTitle !== undefined ? item.animeTitle : (item.sourceTitle || "")
    }

    function episodeLine(item) {
        if (item.episode === undefined) return ""
        const episode = item.season !== undefined ? qsTr("T%1 · E%2").arg(item.season).arg(item.episode)
                                                  : qsTr("Episódio %1").arg(item.episode)
        return item.episodeTitle !== undefined ? episode + " — " + item.episodeTitle : episode
    }

    function joined(parts) {
        return parts.filter(function(part) { return part !== undefined && part !== "" }).join("  ·  ")
    }

    // Pulsing "playing now" marker used on every session card and in the details eyebrow.
    component LiveDot: Rectangle {
        id: dot
        property bool paused: false
        width: 8
        height: 8
        radius: 4
        color: paused ? nowWatching.muted : nowWatching.accent
        SequentialAnimation on opacity {
            running: nowWatching.visible && nowWatching.hasSession && !dot.paused
            // A paused or hidden dot rests fully visible instead of freezing mid-pulse.
            onStopped: dot.opacity = 1
            loops: Animation.Infinite
            NumberAnimation { from: 1; to: 0.25; duration: 900; easing.type: Easing.InOutSine }
            NumberAnimation { from: 0.25; to: 1; duration: 900; easing.type: Easing.InOutSine }
        }
    }

    Rectangle { anchors.fill: parent; color: "#eef2f7" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 18

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Label {
                text: qsTr("HAIKEN ANIME")
                color: nowWatching.accent
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 2.2
            }
            Label {
                text: qsTr("Assistindo agora")
                color: nowWatching.ink
                font.pixelSize: 25
                font.weight: Font.Bold
            }
            AppButton {
                variant: "flat"
                text: qsTr("‹  Biblioteca")
                font.pixelSize: 13
                onClicked: nowWatching.backRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: nowWatching.line
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            // Left: one compact card per open player instance.
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 320
                Layout.horizontalStretchFactor: 2
                color: nowWatching.surface
                radius: 8
                border.color: nowWatching.line

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Label {
                            text: qsTr("Sessões abertas")
                            color: nowWatching.ink
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                        }
                        Label {
                            text: nowWatching.openSessions.length === 1 ? qsTr("1 reprodução em andamento")
                                  : qsTr("%1 reproduções em andamento").arg(nowWatching.openSessions.length)
                            color: nowWatching.muted
                            font.pixelSize: 12
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: nowWatching.line
                    }

                    ListView {
                        id: sessionList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: nowWatching.hasSession
                        clip: true
                        spacing: 10
                        model: nowWatching.openSessions
                        delegate: Rectangle {
                            id: sessionCard
                            required property var modelData
                            readonly property bool selected: modelData.selected === true

                            width: ListView.view.width
                            height: 132
                            radius: 6
                            color: selected ? "#edf4fc" : nowWatching.surface
                            border.color: selected || activeFocus ? nowWatching.accent : "#e0e6ee"
                            border.width: selected || activeFocus ? 2 : 1
                            activeFocusOnTab: true
                            Accessible.role: Accessible.Button
                            Accessible.name: nowWatching.titleOf(modelData)
                            Accessible.onPressAction: nowWatching.controller.SelectSession(modelData.key)

                            TapHandler { onTapped: nowWatching.controller.SelectSession(sessionCard.modelData.key) }
                            Keys.onReturnPressed: nowWatching.controller.SelectSession(modelData.key)
                            Keys.onSpacePressed: nowWatching.controller.SelectSession(modelData.key)

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 14

                                Rectangle {
                                    Layout.preferredWidth: 76
                                    Layout.preferredHeight: 108
                                    radius: 4
                                    color: "#dbe7f4"
                                    clip: true
                                    Image {
                                        anchors.fill: parent
                                        source: nowWatching.coverOf(sessionCard.modelData)
                                        fillMode: Image.PreserveAspectCrop
                                        asynchronous: true
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignVCenter
                                    spacing: 5

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 7
                                        LiveDot { paused: sessionCard.modelData.paused === true }
                                        Label {
                                            Layout.fillWidth: true
                                            text: nowWatching.joined([(sessionCard.modelData.source || "").toUpperCase(),
                                                                      sessionCard.modelData.user,
                                                                      sessionCard.modelData.device,
                                                                      sessionCard.modelData.paused === true
                                                                          ? qsTr("Pausado").toUpperCase() : undefined])
                                            color: nowWatching.accent
                                            font.pixelSize: 10
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 1.2
                                            elide: Text.ElideRight
                                        }
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: nowWatching.titleOf(sessionCard.modelData)
                                        color: nowWatching.ink
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: nowWatching.episodeLine(sessionCard.modelData)
                                        color: nowWatching.muted
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }

                    StatePanel {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: !nowWatching.hasSession
                        title: qsTr("Nada sendo reproduzido no momento")
                        description: qsTr("Os episódios em reprodução aparecem aqui.")
                        stateLabel: qsTr("ESTADO VAZIO")
                        accentColor: nowWatching.accent
                    }
                }
            }

            // Right: details of the selected session, in the Home's details-panel pattern.
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 480
                Layout.horizontalStretchFactor: 3
                color: nowWatching.surface
                radius: 8
                border.color: nowWatching.line

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 26
                    spacing: 18

                    RowLayout {
                        Layout.fillWidth: true
                        // Fixed height: the header must not shift when the details button appears.
                        Layout.preferredHeight: 36
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("DETALHES")
                            color: nowWatching.accent
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.5
                        }
                        AppButton {
                            id: animeDetailsButton
                            text: qsTr("Ver detalhes do anime")
                            variant: "flat"
                            visible: nowWatching.current.mediaId !== undefined
                            onClicked: {
                                // Same panel as the Home; selecting there keeps both screens consistent.
                                homeController.SelectMedia(nowWatching.current.mediaId)
                                animeDetails.active = true
                                animeDetails.item.openForItem(animeDetailsButton)
                            }
                        }
                    }

                    StatePanel {
                        visible: !nowWatching.hasSession
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: qsTr("Nenhuma sessão selecionada")
                        description: qsTr("Os detalhes do anime e do episódio aparecem aqui durante a reprodução.")
                        stateLabel: qsTr("ESTADO VAZIO")
                        accentColor: nowWatching.accent
                    }

                    ColumnLayout {
                        visible: nowWatching.hasSession
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 18

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 22

                            Rectangle {
                                Layout.alignment: Qt.AlignTop
                                Layout.preferredWidth: 150
                                Layout.preferredHeight: 214
                                radius: 6
                                color: nowWatching.accentSoft
                                clip: true
                                Image {
                                    anchors.fill: parent
                                    source: nowWatching.coverOf(nowWatching.current)
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                                spacing: 10

                                TextEdit {
                                    Layout.fillWidth: true
                                    text: nowWatching.titleOf(nowWatching.current)
                                    color: nowWatching.ink
                                    font.pixelSize: 20
                                    font.weight: Font.Bold
                                    readOnly: true
                                    selectByMouse: true
                                    wrapMode: TextEdit.Wrap
                                    Accessible.name: qsTr("Título em reprodução")
                                }

                                Rectangle {
                                    visible: nowWatching.current.associated === false
                                    implicitWidth: unassociatedLabel.implicitWidth + 16
                                    implicitHeight: unassociatedLabel.implicitHeight + 6
                                    radius: 4
                                    color: nowWatching.surfaceSoft
                                    border.color: nowWatching.line
                                    Label {
                                        id: unassociatedLabel
                                        anchors.centerIn: parent
                                        text: qsTr("não associado à biblioteca")
                                        color: nowWatching.muted
                                        font.pixelSize: 11
                                    }
                                }

                                ColumnLayout {
                                    visible: nowWatching.current.progress !== undefined
                                    spacing: 4
                                    Label { text: qsTr("Progresso"); color: nowWatching.muted; font.pixelSize: 11 }
                                    Label {
                                        text: nowWatching.current.progress || ""
                                        color: nowWatching.ink
                                        font.weight: Font.DemiBold
                                    }
                                }

                                Label {
                                    visible: nowWatching.current.animeSynopsis !== undefined
                                    Layout.fillWidth: true
                                    text: nowWatching.current.animeSynopsis || ""
                                    color: nowWatching.muted
                                    font.pixelSize: 12
                                    wrapMode: Text.Wrap
                                    maximumLineCount: 6
                                    elide: Text.ElideRight
                                    lineHeight: 1.15
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: nowWatching.line
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                LiveDot { paused: nowWatching.current.paused === true }
                                Label {
                                    text: qsTr("EPISÓDIO")
                                    color: nowWatching.accent
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                    font.letterSpacing: 1.5
                                }
                                Rectangle {
                                    visible: nowWatching.current.paused === true
                                    implicitWidth: pausedLabel.implicitWidth + 14
                                    implicitHeight: pausedLabel.implicitHeight + 6
                                    radius: 4
                                    color: nowWatching.surfaceSoft
                                    border.color: nowWatching.line
                                    Label {
                                        id: pausedLabel
                                        anchors.centerIn: parent
                                        text: qsTr("Pausado")
                                        color: nowWatching.muted
                                        font.pixelSize: 11
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12
                                Label {
                                    Layout.fillWidth: true
                                    text: nowWatching.episodeLine(nowWatching.current)
                                    color: nowWatching.ink
                                    font.pixelSize: 15
                                    font.weight: Font.DemiBold
                                    wrapMode: Text.Wrap
                                }
                                Label {
                                    Layout.alignment: Qt.AlignTop
                                    visible: text.length > 0
                                    text: nowWatching.joined([nowWatching.current.duration, nowWatching.current.airDate])
                                    color: nowWatching.muted
                                    font.pixelSize: 12
                                }
                            }

                            Label {
                                visible: nowWatching.current.summary !== undefined
                                Layout.fillWidth: true
                                text: nowWatching.current.summary || ""
                                color: nowWatching.ink
                                font.pixelSize: 12
                                wrapMode: Text.Wrap
                                maximumLineCount: 5
                                elide: Text.ElideRight
                                lineHeight: 1.2
                            }

                            Label {
                                visible: text.length > 0
                                text: nowWatching.joined([nowWatching.current.source, nowWatching.current.user,
                                                          nowWatching.current.device])
                                color: nowWatching.muted
                                font.pixelSize: 11
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }
        }
    }

    // Created on first use: it needs the Home controller, which owns the selected-media details.
    Loader {
        id: animeDetails
        active: false
        sourceComponent: MediaDetailsPanel {
            controller: homeController
            ink: nowWatching.ink
            muted: nowWatching.muted
            line: nowWatching.line
            surface: nowWatching.surface
            surfaceSoft: nowWatching.surfaceSoft
            accent: nowWatching.accent
        }
    }
}
