import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Barra fina de sincronização com a cor da marca AniList. Só aparece durante a
// sincronização e pelos poucos segundos em que o controlador a mantém visível
// após o término (status.visible).
Item {
    id: bar

    required property var status

    readonly property color trackColor: "#d7dee8"
    readonly property color fillColor: status.outcome === "failed" ? "#b42318" : "#02A9FF"
    readonly property bool indeterminate: !status.progressKnown && status.running
                                          && status.outcome === "running"
    readonly property real fraction: status.outcome === "succeeded"
                                     ? 1 : Math.max(0, Math.min(1, status.percent / 100))
    readonly property string labelText: status.outcome === "succeeded" || status.outcome === "failed"
                                        ? status.message
                                        : (status.stageLabel.length > 0
                                           ? qsTr("%1 · %2 itens").arg(status.stageLabel).arg(status.processedItems)
                                           : "")

    visible: status.visible
    implicitHeight: label.visible ? Math.max(3, label.implicitHeight) : 3
    Layout.fillWidth: true

    Accessible.role: Accessible.ProgressBar
    Accessible.name: qsTr("Progresso da sincronização")
    Accessible.description: labelText

    RowLayout {
        anchors.fill: parent
        spacing: 10

        Item {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: 3

            Rectangle {
                anchors.fill: parent
                radius: 1.5
                color: bar.trackColor
            }

            Item {
                anchors.fill: parent
                clip: true

                Rectangle {
                    id: fill
                    height: parent.height
                    radius: 1.5
                    color: bar.fillColor
                    x: bar.indeterminate ? sweep.position : 0
                    width: bar.indeterminate ? parent.width * 0.3 : parent.width * bar.fraction

                    Behavior on width {
                        enabled: !bar.indeterminate
                        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
                    }
                }

                QtObject {
                    id: sweep
                    property real position: 0
                }

                SequentialAnimation {
                    running: bar.visible && bar.indeterminate
                    loops: Animation.Infinite
                    NumberAnimation {
                        target: sweep
                        property: "position"
                        from: -fill.width
                        to: bar.width
                        duration: 1100
                        easing.type: Easing.InOutQuad
                    }
                }
            }
        }

        Label {
            id: label
            visible: bar.labelText.length > 0
            text: bar.labelText
            color: bar.status.outcome === "failed" ? "#b42318" : "#68758a"
            font.pixelSize: 10
            elide: Text.ElideRight
            Layout.maximumWidth: 220
        }
    }
}
