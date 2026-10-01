import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: preview

    property url source: ""
    property Item returnFocusItem: null
    property color ink: "#172033"
    property color muted: "#68758a"
    property color line: "#d7dee8"
    property color surface: "#ffffff"

    function openForSource(focusItem) {
        returnFocusItem = focusItem
        open()
    }

    parent: Overlay.overlay
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: Math.min(720, Math.max(280, (parent ? parent.width : 768) - 48))
    height: Math.min(720, Math.max(280, (parent ? parent.height : 768) - 48))
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
        color: preview.surface
        border.color: preview.line
        border.width: 1
        radius: 8
    }

    contentItem: ColumnLayout {
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 16

            Label {
                Layout.fillWidth: true
                text: qsTr("Capa ampliada")
                color: preview.ink
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }

            Button {
                id: closeButton
                text: qsTr("Fechar")
                flat: true
                onClicked: preview.close()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: preview.line
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 18
            clip: true

            Image {
                id: image
                anchors.fill: parent
                source: preview.source
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                cache: false
            }

            Label {
                anchors.centerIn: parent
                visible: image.status === Image.Error || preview.source.toString().length === 0
                text: qsTr("Capa indisponível")
                color: preview.muted
                font.pixelSize: 13
            }
        }
    }
}
