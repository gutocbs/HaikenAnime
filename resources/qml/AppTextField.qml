import QtQuick 2.15
import QtQuick.Controls.Basic as Basic

// Campo de texto padrão do aplicativo. Substitui o TextField nativo do estilo do
// sistema com o mesmo quadro dos seletores: borda "line", foco de 2 px em "accent".
Basic.TextField {
    id: field

    readonly property Theme theme: Theme {}

    implicitHeight: 44
    leftPadding: 13
    rightPadding: 13
    color: enabled ? theme.ink : theme.muted
    placeholderTextColor: theme.muted
    selectionColor: theme.accentSoft
    selectedTextColor: theme.ink
    selectByMouse: true
    font.pixelSize: 12

    background: Rectangle {
        radius: 5
        color: field.enabled ? theme.surface : theme.surfaceSoft
        border.width: field.activeFocus ? 2 : 1
        border.color: field.activeFocus ? theme.accent : theme.line
    }
}
