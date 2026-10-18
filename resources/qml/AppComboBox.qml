import QtQuick 2.15
import QtQuick.Controls.Basic as Basic

// Seletor padrão do aplicativo, no mesmo desenho dos filtros da biblioteca:
// legenda curta em caixa alta acima do valor e o indicador "⌄" em "accent".
// placeholderText aparece enquanto nenhuma opção estiver selecionada.
Basic.ComboBox {
    id: combo

    property string caption
    property string placeholderText

    readonly property color ink: "#172033"
    readonly property color muted: "#68758a"
    readonly property color line: "#d7dee8"
    readonly property color surface: "#ffffff"
    readonly property color surfaceSoft: "#f6f8fb"
    readonly property color accent: "#315d91"
    readonly property bool hasSelection: currentIndex >= 0
    // Long lists (anos do catálogo sazonal) rolam dentro do popup em vez de crescer até a borda.
    property int visibleItemCount: 8
    readonly property int itemHeight: 38

    implicitHeight: 44
    leftPadding: 12
    rightPadding: 30
    hoverEnabled: true

    background: Rectangle {
        radius: 5
        color: combo.enabled ? combo.surface : combo.surfaceSoft
        border.width: combo.activeFocus ? 2 : 1
        border.color: combo.activeFocus ? combo.accent
                    : combo.hovered && combo.enabled ? "#b9c6d6" : combo.line
    }

    contentItem: Item {
        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: -1

            Text {
                visible: combo.caption.length > 0
                text: combo.caption
                color: combo.muted
                font.pixelSize: 9
                font.weight: Font.DemiBold
                font.letterSpacing: 0.7
            }

            Text {
                width: parent.width
                text: combo.hasSelection ? combo.displayText : combo.placeholderText
                color: combo.enabled && combo.hasSelection ? combo.ink : combo.muted
                font.pixelSize: 12
                font.weight: combo.hasSelection ? Font.DemiBold : Font.Normal
                elide: Text.ElideRight
            }
        }
    }

    indicator: Text {
        x: combo.width - width - 12
        anchors.verticalCenter: parent.verticalCenter
        text: "⌄"
        color: combo.enabled ? combo.accent : combo.muted
        font.pixelSize: 15
    }

    delegate: Basic.ItemDelegate {
        required property int index
        required property var model

        width: combo.popup ? combo.popup.width - combo.popup.leftPadding - combo.popup.rightPadding : combo.width
        implicitHeight: combo.itemHeight
        highlighted: combo.highlightedIndex === index

        contentItem: Text {
            text: model[combo.textRole]
            color: combo.ink
            font.pixelSize: 12
            font.weight: combo.currentIndex === index ? Font.DemiBold : Font.Normal
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            color: parent.highlighted ? "#eaf1f8"
                                      : combo.currentIndex === index ? "#f4f7fb" : combo.surface
        }
    }

    popup: Basic.Popup {
        y: combo.height + 4
        width: combo.width
        implicitHeight: Math.min(optionList.contentHeight, combo.visibleItemCount * combo.itemHeight) + topPadding + bottomPadding
        padding: 4

        // Abre já rolado até a opção selecionada (por exemplo, o ano atual no meio da lista).
        onOpened: optionList.positionViewAtIndex(Math.max(0, combo.currentIndex), ListView.Center)

        contentItem: ListView {
            id: optionList
            clip: true
            implicitHeight: contentHeight
            model: combo.popup.visible ? combo.delegateModel : null
            currentIndex: combo.highlightedIndex
            highlightMoveDuration: 0
            boundsBehavior: Flickable.StopAtBounds

            Basic.ScrollBar.vertical: Basic.ScrollBar {
                policy: optionList.contentHeight > optionList.height ? Basic.ScrollBar.AlwaysOn : Basic.ScrollBar.AlwaysOff
                width: 10
                padding: 2
                contentItem: Rectangle {
                    implicitWidth: 6
                    radius: 3
                    color: parent.pressed ? combo.accent : parent.hovered ? "#9fb0c5" : "#c3cedb"
                }
            }
        }

        background: Rectangle {
            radius: 6
            color: combo.surface
            border.color: combo.line
        }
    }
}
