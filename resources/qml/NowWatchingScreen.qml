import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// "Assistindo agora": what is being recognized right now, one session at a time.
// Anime data comes from the library; episode data is only what the player reported.
// Read-only: the controller pushes every change, this screen never polls or parses players.
Item {
    id: nowWatching

    signal backRequested()

    property var controller: nowWatchingController
    readonly property var current: controller ? controller.current : ({})
    readonly property bool hasSession: controller !== null && controller.active

    readonly property color ink: "#172033"
    readonly property color muted: "#68758a"
    readonly property color line: "#d7dee8"
    readonly property color surface: "#ffffff"
    readonly property color surfaceSoft: "#f6f8fb"
    readonly property color accent: "#315d91"
    readonly property color accentSoft: "#e2ebf6"
    readonly property string placeholderCover: "qrc:/qt/qml/HaikenAnime/resources/images/cover-placeholder.svg"

    function statusLabel(key) {
        switch (key) {
        case "current": return qsTr("Assistindo")
        case "planning": return qsTr("Planejado")
        case "on-hold": return qsTr("Pausado")
        case "dropped": return qsTr("Abandonado")
        case "completed": return qsTr("Completo")
        }
        return ""
    }

    function selectedSessionIndex() {
        const sessions = controller ? controller.sessions : []
        for (let index = 0; index < sessions.length; ++index) {
            if (sessions[index].key === controller.selectedSessionKey) return index
        }
        return -1
    }

    function animeFacts() {
        return [current.progress !== undefined ? qsTr("Progresso %1").arg(current.progress) : undefined,
                statusLabel(current.listStatus) || undefined]
               .filter(function(part) { return part !== undefined })
    }

        function episodeLine() {
        const episode = current.season !== undefined
                ? qsTr("T%1 · E%2").arg(current.season).arg(current.episode)
                : qsTr("Episódio %1").arg(current.episode)
        return current.episodeTitle !== undefined ? episode + " — " + current.episodeTitle : episode
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

        // Several players at once (e.g. mpv here and another selected Plex user): pick one.
        RowLayout {
            visible: nowWatching.hasSession && nowWatching.controller.sessions.length > 1
            Layout.fillWidth: true
            spacing: 10
            Label {
                text: qsTr("Sessão")
                color: nowWatching.muted
                font.pixelSize: 12
            }
            ComboBox {
                id: sessionSelector
                Layout.preferredWidth: 380
                model: nowWatching.controller ? nowWatching.controller.sessions : []
                textRole: "label"
                valueRole: "key"
                currentIndex: nowWatching.selectedSessionIndex()
                onActivated: nowWatching.controller.SelectSession(currentValue)
                Accessible.name: qsTr("Sessão em reprodução")
            }
        }

        Rectangle {
            visible: nowWatching.hasSession
            Layout.fillWidth: true
            Layout.maximumWidth: 920
            implicitHeight: card.implicitHeight + 48
            color: nowWatching.surface
            radius: 7
            border.color: nowWatching.line

            RowLayout {
                id: card
                anchors.fill: parent
                anchors.margins: 24
                spacing: 24

                Rectangle {
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: 168
                    Layout.preferredHeight: 244
                    color: nowWatching.surfaceSoft
                    radius: 5
                    border.color: nowWatching.line
                    clip: true

                    Image {
                        anchors.fill: parent
                        anchors.margins: 1
                        source: nowWatching.current.coverUrl !== undefined ? nowWatching.current.coverUrl
                                                                           : nowWatching.placeholderCover
                        fillMode: Image.PreserveAspectCrop
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: 10

                    // Signature: the live marker. The only animation on this screen.
                    RowLayout {
                        spacing: 8
                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: nowWatching.accent
                            SequentialAnimation on opacity {
                                running: nowWatching.visible && nowWatching.hasSession
                                loops: Animation.Infinite
                                NumberAnimation { from: 1; to: 0.25; duration: 900; easing.type: Easing.InOutSine }
                                NumberAnimation { from: 0.25; to: 1; duration: 900; easing.type: Easing.InOutSine }
                            }
                        }
                        Label {
                            text: qsTr("ASSISTINDO AGORA · %1").arg((nowWatching.current.source || "").toUpperCase())
                            color: nowWatching.accent
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.8
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: nowWatching.current.animeTitle !== undefined ? nowWatching.current.animeTitle
                                                                           : (nowWatching.current.sourceTitle || "")
                        color: nowWatching.ink
                        font.pixelSize: 24
                        font.weight: Font.Bold
                        wrapMode: Text.Wrap
                    }

                    RowLayout {
                        spacing: 8
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
                        Label {
                            readonly property var parts: nowWatching.animeFacts()
                            visible: parts.length > 0
                            text: parts.join("  ·  ")
                            color: nowWatching.muted
                            font.pixelSize: 12
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.topMargin: 6
                        implicitHeight: episodeBlock.implicitHeight + 28
                        color: nowWatching.surfaceSoft
                        radius: 7
                        border.color: nowWatching.line

                        ColumnLayout {
                            id: episodeBlock
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 6

                            Label {
                                Layout.fillWidth: true
                                text: nowWatching.current.episode !== undefined ? nowWatching.episodeLine() : ""
                                color: nowWatching.ink
                                font.pixelSize: 15
                                font.weight: Font.DemiBold
                                wrapMode: Text.Wrap
                            }
                            Label {
                                readonly property var parts: [nowWatching.current.duration, nowWatching.current.airDate]
                                                             .filter(function(part) { return part !== undefined })
                                visible: parts.length > 0
                                text: parts.join("  ·  ")
                                color: nowWatching.muted
                                font.pixelSize: 12
                            }
                            Label {
                                visible: nowWatching.current.summary !== undefined
                                Layout.fillWidth: true
                                text: nowWatching.current.summary || ""
                                color: nowWatching.ink
                                font.pixelSize: 12
                                wrapMode: Text.Wrap
                                maximumLineCount: 4
                                elide: Text.ElideRight
                                lineHeight: 1.2
                            }
                        }
                    }

                    Label {
                        readonly property var parts: [nowWatching.current.user, nowWatching.current.device]
                                                     .filter(function(part) { return part !== undefined })
                        visible: parts.length > 0
                        text: parts.join("  ·  ")
                        color: nowWatching.muted
                        font.pixelSize: 11
                    }
                }
            }
        }

        Rectangle {
            visible: !nowWatching.hasSession
            Layout.fillWidth: true
            Layout.maximumWidth: 920
            implicitHeight: 140
            color: nowWatching.surfaceSoft
            radius: 7
            border.color: nowWatching.line

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 6
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Nada sendo reproduzido no momento")
                    color: nowWatching.ink
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Assista um episódio pelo mpv ou pelo Plex para vê-lo aqui.")
                    color: nowWatching.muted
                    font.pixelSize: 11
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
