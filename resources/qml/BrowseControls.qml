import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts 1.15

RowLayout {
    id: controls

    property var controller
    property color ink: "#172033"
    property color muted: "#68758a"
    property color line: "#d7dee8"
    property color surface: "#ffffff"
    property color accent: "#315d91"
    spacing: 8

    component BrowseComboBox: Basic.ComboBox {
        id: combo
        property string caption
        implicitHeight: 44
        leftPadding: 12
        rightPadding: 30

        background: Rectangle {
            radius: 5
            color: controls.surface
            border.width: combo.activeFocus ? 2 : 1
            border.color: combo.activeFocus ? controls.accent : controls.line
        }

        contentItem: Item {
            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: -1

                Text {
                    text: combo.caption
                    color: controls.muted
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.7
                }

                Text {
                    width: parent.width
                    text: combo.displayText
                    color: controls.ink
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
            }
        }

        indicator: Text {
            x: combo.width - width - 12
            anchors.verticalCenter: parent.verticalCenter
            text: "⌄"
            color: controls.accent
            font.pixelSize: 15
        }

        delegate: Basic.ItemDelegate {
            required property int index
            required property var model

            width: combo.popup ? combo.popup.width : combo.width
            implicitHeight: 38
            highlighted: combo.highlightedIndex === index

            contentItem: Text {
                text: model[combo.textRole]
                color: controls.ink
                font.pixelSize: 12
                font.weight: combo.currentIndex === index ? Font.DemiBold : Font.Normal
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            background: Rectangle {
                color: parent.highlighted ? "#eaf1f8"
                                          : combo.currentIndex === index ? "#f4f7fb" : controls.surface
            }
        }
    }

    BrowseComboBox {
        id: listFilter
        Layout.preferredWidth: 138
        caption: qsTr("LISTA")
        model: controls.controller.availableListOptions
        textRole: "label"
        valueRole: "key"
        currentIndex: indexOfValue(controls.controller.activeListFilter)
        onActivated: controls.controller.SetListFilter(currentValue)
        Accessible.name: qsTr("Selecionar lista")
    }

    BrowseComboBox {
        id: sortOrder
        Layout.preferredWidth: 168
        caption: qsTr("ORDENAR")
        model: controls.controller.availableSortOptions
        textRole: "label"
        valueRole: "key"
        currentIndex: indexOfValue(controls.controller.activeSort)
        onActivated: controls.controller.SetSort(currentValue)
        Accessible.name: qsTr("Ordenar biblioteca")
    }

    Basic.TextField {
        id: searchField
        Layout.fillWidth: true
        Layout.minimumWidth: 180
        implicitHeight: 44
        text: controls.controller.searchQuery
        placeholderText: qsTr("Buscar por título...")
        selectByMouse: true
        leftPadding: 14
        rightPadding: 14
        color: controls.ink
        font.pixelSize: 12
        Accessible.name: qsTr("Buscar mídia por título")
        onTextEdited: searchTimer.restart()

        background: Rectangle {
            radius: 5
            color: controls.surface
            border.width: searchField.activeFocus ? 2 : 1
            border.color: searchField.activeFocus ? controls.accent : controls.line
        }

        Timer {
            id: searchTimer
            interval: 180
            repeat: false
            onTriggered: controls.controller.SetSearchQuery(searchField.text)
        }
    }

    Connections {
        target: controls.controller
        function onBrowseCriteriaChanged() {
            if (searchField.text !== controls.controller.searchQuery)
                searchField.text = controls.controller.searchQuery
        }
    }
}
