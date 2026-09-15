import QtQuick 2.0
import QtQuick.Controls 2.3

Item {
    width: 1020
    height: 814

    Column {
        id: column
        anchors.fill: parent

        Row {
            id: row
            width: parent.width
            height: 400

            Column {
                id: column1
                width: 500
                height: parent.height

                Row {
                    id: row1
                    width: parent.width
                    height: 50

                    Item {
                        id: element
                        width: parent.width
                        height: parent.height

                        Label {
                            id: label
                            text: qsTr("Rules")
                            renderType: Text.NativeRendering
                            font.pointSize: 15
                            font.family: "Tahoma"
                            verticalAlignment: Text.AlignVCenter
                            anchors.fill: parent
                        }
                    }
                }

                Row {
                    id: row2
                    width: parent.width
                    height: 350
                    Item {
                        id: element1
                        width: parent.width
                        height: parent.height

                        Rectangle {
                            id: rectangle
                            color: "#ffffff"
                            anchors.fill: parent
                        }
                        ListView {
                            id: listaDiretorios
                            model: ListModel {
                                id: modeloLista
                                ListElement {
                                    name: "Grey"
                                    colorCode: "grey"
                                }

                                ListElement {
                                    name: "Red"
                                    colorCode: "red"
                                }

                                ListElement {
                                    name: "Blue"
                                    colorCode: "blue"
                                }

                                ListElement {
                                    name: "Green"
                                    colorCode: "green"
                                }
                            }
                            delegate: Item {
                                x: 5
                                width: 80
                                height: 40
                                Row {
                                    id: row7
                                    spacing: 10
                                    Rectangle {
                                        width: 40
                                        height: 40
                                        color: colorCode
                                    }

                                    Text {
                                        text: name
                                        font.bold: true
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                }
                            }
                            anchors.fill: parent
                            highlightRangeMode: ListView.NoHighlightRange
                            highlight: Rectangle {
                                color: "#ff0000"
                                radius: 20
                            }
                            focus: true
                        }
                    }
                }
            }

            Column {
                id: column2
                width: parent.width-column1.width
                height: 400

                Row {
                    id: row3
                    width: parent.width
                    height: 50

                    Column {
                        id: column3
                        width: 200
                        height: parent.height

                        Item {
                            id: element2
                            width: parent.width
                            height: parent.height

                            Button {
                                id: button
                                text: qsTr("Add new rule")
                            }
                        }
                    }

                    Column {
                        id: column4
                        width: 200
                        height: parent.height
                        Item {
                            id: element3
                            width: parent.width
                            height: parent.height

                            Button {
                                id: button1
                                text: qsTr("Remove rule")
                            }
                        }
                    }
                }

                Row {
                    id: row4
                    width: parent.width
                    height: 50
                    Column {
                        id: column5
                        width: 200
                        height: parent.height
                        Item {
                            id: element4
                            width: parent.width
                            height: parent.height

                            Button {
                                id: button2
                                text: qsTr("Enable/Disable rule")
                            }
                        }
                    }

                    Column {
                        id: column6
                        width: 200
                        height: parent.height
                        Item {
                            id: element5
                            width: parent.width
                            height: parent.height

                            Button {
                                id: button3
                                text: qsTr("Check rule")
                            }
                        }
                    }
                }

                Row {
                    id: row5
                    width: parent.width
                    height: 50
                    Column {
                        id: column7
                        width: 200
                        height: parent.height
                        Item {
                            id: element6
                            width: parent.width
                            height: parent.height

                            TextField {
                                id: textField
                                text: qsTr("")
                                placeholderText: "Rule name"
                            }
                        }
                    }
                }

                Row {
                    id: row6
                    width: parent.width
                    height: 50
                    Column {
                        id: column9
                        width: 200
                        height: parent.height
                        Item {
                            id: element8
                            width: parent.width
                            height: parent.height

                            ComboBox {
                                id: comboBox
                            }
                        }
                    }

                    Column {
                        id: column10
                        width: 200
                        height: parent.height
                        Item {
                            id: element9
                            width: parent.width
                            height: parent.height

                            TextField {
                                id: textField1
                                text: qsTr("")
                                placeholderText: "Rule"
                            }
                        }
                    }
                }

                Row {
                    id: row8
                    width: parent.width
                    height: 50
                    Column {
                        id: column11
                        width: 200
                        height: parent.height
                        Item {
                            id: element10
                            width: parent.width
                            height: parent.height

                            Button {
                                id: button4
                                text: qsTr("Save rule")
                            }
                        }
                    }
                }
            }
        }

        Row {
            id: row9
            width: parent.width
            height: parent.height-row.height

            Column {
                id: column8
                width: 200
                height: parent.height

                Row {
                    id: row10
                    width: parent.width
                    height: 50

                    Item {
                        id: element7
                        width: parent.width
                        height: parent.height

                        Button {
                            id: button5
                            text: qsTr("Watching")
                        }
                    }
                }

                Row {
                    id: row11
                    width: parent.width
                    height: 50
                    Item {
                        id: element11
                        width: parent.width
                        height: parent.height
                        Button {
                            id: button6
                            text: qsTr("Completed")
                        }
                    }
                }

                Row {
                    id: row12
                    width: parent.width
                    height: 50
                    Item {
                        id: element12
                        width: parent.width
                        height: parent.height
                        Button {
                            id: button7
                            text: qsTr("On Hold")
                        }
                    }
                }

                Row {
                    id: row13
                    width: parent.width
                    height: 50
                    Item {
                        id: element13
                        width: parent.width
                        height: parent.height
                        Button {
                            id: button8
                            text: qsTr("Dropped")
                        }
                    }
                }

                Row {
                    id: row14
                    width: parent.width
                    height: 50
                    Item {
                        id: element14
                        width: parent.width
                        height: parent.height
                        Button {
                            id: button9
                            text: qsTr("Plan to Watch")
                        }
                    }
                }
            }

            Column {
                id: column12
                width: 310
                height: parent.height


                Row {
                    id: row17
                    width: parent.width
                    height: 50

                    Item {
                        id: element17
                        width: parent.width
                        height: parent.height

                        Label {
                            id: label1
                            text: qsTr("Animes not following the rules")
                            verticalAlignment: Text.AlignVCenter
                            anchors.fill: parent
                        }
                    }
                }

                Row {
                    id: row18
                    width: parent.width
                    height: parent.height-row17.height

                    Item {
                        id: element20
                        width: parent.width
                        height: parent.height

                        Rectangle {
                            id: rectangle1
                            color: "#ffffff"
                            anchors.fill: parent
                        }
                        ListView {
                            id: listaDiretorios3
                            x: 0
                            y: 0
                            width: 200
                            model: ListModel {
                                id: modeloLista3
                                ListElement {
                                    name: "Grey"
                                    colorCode: "grey"
                                }

                                ListElement {
                                    name: "Red"
                                    colorCode: "red"
                                }

                                ListElement {
                                    name: "Blue"
                                    colorCode: "blue"
                                }

                                ListElement {
                                    name: "Green"
                                    colorCode: "green"
                                }
                            }
                            delegate: Item {
                                x: 5
                                width: 80
                                height: 40
                                Row {
                                    id: row23
                                    spacing: 10
                                    Rectangle {
                                        width: 40
                                        height: 40
                                        color: colorCode
                                    }

                                    Text {
                                        text: name
                                        font.bold: true
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                }
                            }
                            anchors.fill: parent
                            highlightRangeMode: ListView.NoHighlightRange
                            highlight: Rectangle {
                                color: "#ff0000"
                                radius: 20
                            }
                            focus: true
                        }
                    }
                }
            }

            Column {
                id: column13
                width: 200
                height: parent.height

                Row {
                    id: row15
                    width: parent.width
                    height: 50
                    Item {
                        id: element15
                        width: parent.width
                        height: parent.height
                        Button {
                            id: button10
                            text: qsTr("Insert")
                        }
                    }
                }

                Row {
                    id: row16
                    width: parent.width
                    height: 50
                    Item {
                        id: element16
                        width: parent.width
                        height: parent.height
                        Button {
                            id: button11
                            text: qsTr("Remove")
                        }
                    }
                }
            }

            Column {
                id: column14
                width: 310
                height: parent.height

                Row {
                    id: row19
                    width: parent.width
                    height: 50
                    Item {
                        id: element18
                        width: parent.width
                        height: parent.height
                        Label {
                            id: label2
                            text: qsTr("Animes following the rules")
                            anchors.fill: parent
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                Row {
                    id: row21
                    width: parent.width
                    height: parent.height-row17.height

                    Item {
                        id: element19
                        width: parent.width
                        height: parent.height


                        Rectangle {
                            id: rectangle2
                            color: "#ffffff"
                            anchors.fill: parent
                        }
                        ListView {
                            id: listaDiretorios2
                            x: 0
                            y: 0
                            width: 200
                            model: ListModel {
                                id: modeloLista2
                                ListElement {
                                    name: "Grey"
                                    colorCode: "grey"
                                }

                                ListElement {
                                    name: "Red"
                                    colorCode: "red"
                                }

                                ListElement {
                                    name: "Blue"
                                    colorCode: "blue"
                                }

                                ListElement {
                                    name: "Green"
                                    colorCode: "green"
                                }
                            }
                            delegate: Item {
                                x: 5
                                width: 80
                                height: 40
                                Row {
                                    id: row22
                                    spacing: 10
                                    Rectangle {
                                        width: 40
                                        height: 40
                                        color: colorCode
                                    }

                                    Text {
                                        text: name
                                        font.bold: true
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                }
                            }
                            anchors.fill: parent
                            highlightRangeMode: ListView.NoHighlightRange
                            highlight: Rectangle {
                                color: "#ff0000"
                                radius: 20
                            }
                            focus: true
                        }
                    }
                }
            }
        }
    }

}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.75}D{i:9;anchors_height:200;anchors_width:200}D{i:10;anchors_height:160;anchors_width:110}
D{i:75;anchors_height:200;anchors_width:200}D{i:76;anchors_height:160;anchors_width:110}
D{i:74;anchors_height:160;anchors_width:110}D{i:101;anchors_height:200;anchors_width:200}
D{i:102;anchors_height:160;anchors_width:110}D{i:99;anchors_height:160;anchors_width:110}
D{i:1;anchors_height:400;anchors_width:200}
}
##^##*/
