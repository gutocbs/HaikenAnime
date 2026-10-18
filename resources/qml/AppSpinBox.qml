import QtQuick 2.15
import QtQuick.Controls.Basic as Basic

// Seletor numérico padrão do aplicativo. Mesmo quadro do AppTextField, com os
// botões − e + embutidos nas bordas no lugar das setas nativas do sistema.
Basic.SpinBox {
    id: spin

    readonly property color ink: "#172033"
    readonly property color muted: "#68758a"
    readonly property color line: "#d7dee8"
    readonly property color surface: "#ffffff"
    readonly property color surfaceSoft: "#f6f8fb"
    readonly property color accent: "#315d91"
    readonly property color accentSoft: "#e2ebf6"
    readonly property color accentPressed: "#cfdcec"
    readonly property int stepperWidth: 38
    readonly property bool canDecrease: enabled && value > Math.min(from, to)
    readonly property bool canIncrease: enabled && value < Math.max(from, to)

    implicitWidth: 156
    implicitHeight: 44
    leftPadding: stepperWidth
    rightPadding: stepperWidth
    font.pixelSize: 12
    font.weight: Font.DemiBold
    hoverEnabled: true

    contentItem: TextInput {
        text: spin.displayText
        font: spin.font
        color: spin.enabled ? spin.ink : spin.muted
        selectionColor: spin.accentSoft
        selectedTextColor: spin.ink
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !spin.editable
        validator: spin.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        selectByMouse: true
    }

    // Steppers are inset by the frame border so their hover fill never covers the outline.
    down.indicator: Rectangle {
        x: spin.mirrored ? spin.width - width - 1 : 1
        y: 1
        implicitWidth: spin.stepperWidth
        height: spin.height - 2
        radius: 4
        color: !spin.canDecrease ? "transparent"
             : spin.down.pressed ? spin.accentPressed
             : spin.down.hovered ? spin.accentSoft : "transparent"

        Text {
            anchors.centerIn: parent
            text: "−"
            color: spin.canDecrease ? spin.accent : spin.line
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }
    }

    up.indicator: Rectangle {
        x: spin.mirrored ? 1 : spin.width - width - 1
        y: 1
        implicitWidth: spin.stepperWidth
        height: spin.height - 2
        radius: 4
        color: !spin.canIncrease ? "transparent"
             : spin.up.pressed ? spin.accentPressed
             : spin.up.hovered ? spin.accentSoft : "transparent"

        Text {
            anchors.centerIn: parent
            text: "+"
            color: spin.canIncrease ? spin.accent : spin.line
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }
    }

    background: Rectangle {
        radius: 5
        color: spin.enabled ? spin.surface : spin.surfaceSoft
        border.width: spin.activeFocus ? 2 : 1
        border.color: spin.activeFocus ? spin.accent : spin.line
    }
}
