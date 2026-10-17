import QtQuick 2.15
import QtQuick.Controls.Basic as Basic

// Botão padrão do aplicativo. Substitui o botão nativo do estilo do sistema e
// mantém o mesmo vocabulário visual em todas as telas: hover, pressionado,
// foco de teclado (anel de 2 px em "ink") e desabilitado.
//
// variant: "primary" | "secondary" | "flat" | "danger"
// baseColor: cor opcional que substitui o preenchimento de "primary" (usada
// apenas pelos links externos que carregam a cor de cada serviço).
Basic.Button {
    id: button

    property string variant: "primary"
    property color baseColor: "transparent"
    readonly property bool hasBaseColor: baseColor.a > 0

    readonly property color ink: "#172033"
    readonly property color muted: "#68758a"
    readonly property color line: "#d7dee8"
    readonly property color surface: "#ffffff"
    readonly property color surfaceSoft: "#f6f8fb"
    readonly property color accent: "#315d91"
    readonly property color accentSoft: "#e2ebf6"
    readonly property color danger: "#b42318"

    readonly property bool isFlat: variant === "flat"
    readonly property bool isSecondary: variant === "secondary"
    readonly property bool isDanger: variant === "danger"
    readonly property bool isPrimary: !isFlat && !isSecondary && !isDanger

    readonly property color fillColor: {
        if (!enabled) return isFlat ? "transparent" : surfaceSoft
        if (isFlat) return down ? "#cfdcec" : hovered ? accentSoft : "transparent"
        if (isSecondary) return down ? accentSoft : hovered ? surfaceSoft : surface
        if (isDanger) return down ? "#8f1c12" : hovered ? "#c9362a" : danger
        if (hasBaseColor) return down ? Qt.darker(baseColor, 1.2)
                                      : hovered ? Qt.lighter(baseColor, 1.08) : baseColor
        return down ? "#24466d" : hovered ? "#3c6eaa" : accent
    }
    readonly property color outlineColor: {
        if (activeFocus) return ink
        if (!enabled) return isFlat ? "transparent" : line
        if (isFlat) return "transparent"
        if (isSecondary) return hovered || down ? accent : line
        if (isDanger) return danger
        if (hasBaseColor) return Qt.darker(baseColor, 1.35)
        return accent
    }
    readonly property color labelColor: {
        if (!enabled) return muted
        if (isFlat || isSecondary) return isSecondary ? ink : accent
        return surface
    }

    implicitHeight: 34
    implicitWidth: Math.max(implicitContentWidth + leftPadding + rightPadding, 34)
    leftPadding: 14
    rightPadding: 14
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    font.pixelSize: 13
    font.weight: isFlat ? Font.Normal : Font.DemiBold

    Accessible.role: Accessible.Button
    Accessible.name: text

    HoverHandler {
        enabled: button.enabled
        cursorShape: Qt.PointingHandCursor
    }

    contentItem: Text {
        text: button.text
        color: button.labelColor
        font: button.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 5
        color: button.fillColor
        border.width: button.activeFocus ? 2 : (button.isFlat ? 0 : 1)
        border.color: button.outlineColor
    }
}
