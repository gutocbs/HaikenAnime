import QtQuick 2.0
import QtQuick.Controls 2.3

Item {
    id: element
    width: 420
    height: 300

    Column {
        id: column
        width: parent.width
        height: parent.height

        Row {
            id: row
            width: parent.width
            height: parent.height/5

            Item {
                id: element5
                width: parent.width
                height: parent.height

                Button {
                    id: button1
                    visible: true
                    text: "Add new name"
                    anchors.fill: parent
                    font.pixelSize: 20
                    hoverEnabled: false
                    display: AbstractButton.TextOnly
                    spacing: 0
                    contentItem: Text {
                        opacity: enabled ? 1.0 : 0.3
                        color: "#000000"
                        text: button1.text
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.WordWrap
                        font: button1.font
                        fontSizeMode: Text.Fit
                    }
                    focusPolicy: Qt.NoFocus
                    flat: true
                    clip: false
                    enabled: true
                    background: Rectangle {
                        opacity: enabled ? 1 : 0.3
                        color: "transparent"
                        radius: 2
                        border.color: "transparent"
                        border.width: 1
                        implicitHeight: 40
                        implicitWidth: 100
                    }
                }
            }
        }

        Row {
            id: row1
            width: parent.width
            height: parent.height/5

            Column {
                id: column1
                width: parent.width*0.8
                height: parent.height

                Item {
                    id: element1
                    width: parent.width
                    height: parent.height

                    TextField {
                        id: inovoNome
                        text: qsTr("")
                        anchors.fill: parent
                        horizontalAlignment: Text.AlignLeft
                        font.family: "Tahoma"
                        renderType: Text.NativeRendering
                        rightPadding: 6
                        selectByMouse: true
                        placeholderText: "New name"
                        font.pointSize: 10
                    }
                }
            }

            Column {
                id: column2
                width: parent.width-column1.width
                height: parent.height

                Item {
                    id: element2
                    width: parent.width
                    height: parent.height

                    Button {
                        id: iconfirmarNovoNome
                        x: 3
                        y: 45
                        width: parent.width-parent.width/20
                        height: parent.width
                        text: qsTr("Confirm")
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: mainClass.setMediaCustomName(inovoNome.text);
                    }
                }
            }
        }

        Row {
            id: row2
            width: parent.width
            height: parent.height/5

            Item {
                id: element6
                width: parent.width
                height: parent.height
                Button {
                    id: button2
                    visible: true
                    text: "Add new name"
                    anchors.fill: parent
                    font.pixelSize: 20
                    hoverEnabled: false
                    spacing: 0
                    display: AbstractButton.TextOnly
                    contentItem: Text {
                        opacity: enabled ? 1.0 : 0.3
                        color: "#000000"
                        text: "Select folder"
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.WordWrap
                        font: button2.font
                        fontSizeMode: Text.Fit
                    }
                    focusPolicy: Qt.NoFocus
                    flat: true
                    enabled: true
                    clip: false
                    background: Rectangle {
                        opacity: enabled ? 1 : 0.3
                        color: "transparent"
                        radius: 2
                        border.color: "transparent"
                        border.width: 1
                        implicitHeight: 40
                        implicitWidth: 100
                    }
                }
            }
        }

        Row {
            id: row3
            width: parent.width
            height: parent.height/5

            Column {
                id: column3
                width: parent.width*0.8
                height: parent.height
                Item {
                    id: element3
                    width: parent.width
                    height: parent.height

                    Button {
                        id: button3
                        visible: true
                        text: "Add new name"
                        anchors.fill: parent
                        font.pixelSize: 20
                        hoverEnabled: false
                        display: AbstractButton.TextOnly
                        spacing: 0
                        contentItem: Text {
                            opacity: enabled ? 1.0 : 0.3
                            color: "#000000"
                            text: "Select folder"
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            font: button3.font
                            fontSizeMode: Text.Fit
                        }
                        focusPolicy: Qt.NoFocus
                        flat: true
                        clip: false
                        enabled: true
                        background: Rectangle {
                            opacity: enabled ? 1 : 0.3
                            color: "#00000000"
                            radius: 2
                            border.color: "#00000000"
                            border.width: 1
                            implicitHeight: 40
                            implicitWidth: 100
                        }
                    }
                }
            }

            Column {
                id: column4
                width: parent.width-column1.width
                height: parent.height
                Item {
                    id: element4
                    width: parent.width
                    height: parent.height
                    Button {
                        id: botaoConfirmarLista1
                        x: 3
                        y: 45
                        width: parent.width-parent.width/20
                        height: parent.width
                        text: qsTr("Search")
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: stackViewSupportButtons.push("CustomFileDialogAnimeSupportButtonsFolder.qml")
                    }
                }
            }
        }

        Row {
            id: row4
            width: parent.width
            height: parent.height/5

            Column {
                id: column7
                width: parent.width*0.7
                height: parent.height

                Item {
                    id: element10
                    x: 0
                    width: parent.width
                    height: parent.height

                    Button {
                        id: button4
                        visible: true
                        text: "Add new name"
                        anchors.fill: parent
                        font.pixelSize: 20
                        hoverEnabled: false
                        spacing: 0
                        display: AbstractButton.TextOnly
                        contentItem: Text {
                            opacity: enabled ? 1.0 : 0.3
                            color: "#000000"
                            text: "Remove from list"
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            font: button4.font
                            fontSizeMode: Text.Fit
                        }
                        focusPolicy: Qt.NoFocus
                        flat: true
                        enabled: true
                        clip: false
                        background: Rectangle {
                            opacity: enabled ? 1 : 0.3
                            color: "#00000000"
                            radius: 2
                            border.color: "#00000000"
                            border.width: 1
                            implicitHeight: 40
                            implicitWidth: 100
                        }
                    }
                }
            }

            Column {
                id: column8
                width: parent.width-column7.width
                height: parent.height
                Item {
                    id: element9
                    width: parent.width
                    height: parent.height

                    DelayButton {
                        id: ibotaoRemoveFromList
                        x: 3
                        y: 45
                        width: parent.width-parent.width/20
                        height: parent.width
                        text: qsTr("Remove")
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: mainClass.removeMediaFromList();
                    }
                }
            }

        }
    }
    StackView {
        id: stackViewSupportButtons
    }

}


