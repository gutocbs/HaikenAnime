import QtQuick 2.15
import QtQuick.Controls.Basic as Basic

// Campo de texto padrão do aplicativo. Substitui o TextField nativo do estilo do
// sistema com o mesmo quadro dos seletores: borda "line", foco de 2 px em "accent".
Basic.TextField {
    id: field

    readonly property color ink: "#172033"
    readonly property color muted: "#68758a"
    readonly property color line: "#d7dee8"
    readonly property color surface: "#ffffff"
    readonly property color surfaceSoft: "#f6f8fb"
    readonly property color accent: "#315d91"
    readonly property color accentSoft: "#e2ebf6"

    implicitHeight: 44
    leftPadding: 13
    rightPadding: 13
    color: enabled ? ink : muted
    placeholderTextColor: muted
    selectionColor: accentSoft
    selectedTextColor: ink
    selectByMouse: true
    font.pixelSize: 12

    background: Rectangle {
        radius: 5
        color: field.enabled ? field.surface : field.surfaceSoft
        border.width: field.activeFocus ? 2 : 1
        border.color: field.activeFocus ? field.accent : field.line
    }
}
