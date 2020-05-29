import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Dialogs 1.3

Item {
    width: 1020
    height: 814

    Row {
        id: row
        anchors.fill: parent

        Column {
            id: column
            width: 400
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
                        text: qsTr("Anime Directory")
                        renderType: Text.NativeRendering
                        font.pointSize: 12
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                        anchors.fill: parent
                    }
                }
            }

            Row {
                id: row2
                width: parent.width
                height: 200
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
                        x: 0
                        y: 0
                        highlightRangeMode: ListView.NoHighlightRange
                        anchors.fill: parent
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
                                    anchors.verticalCenter: parent.verticalCenter
                                    font.bold: true
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: listaDiretorios.currentIndex = index
                            }
                        }
                        highlight: Rectangle { color: "red"; radius: 20 }
                        focus: true
                        onCurrentItemChanged: console.log(model.get(listaDiretorios.currentIndex).name + ' selected')
                    }
                }
            }

            Row {
                id: row3
                width: parent.width
                height: 50

                Column {
                    id: column2
                    width: 300
                    height: parent.height

                    Item {
                        id: element2
                        width: 300
                        height: parent.height

                        Button {
                            id: button
                            x: 195
                            text: qsTr("Add")
                            onClicked: stackViewDiretorios.push("CustomFileDialog.qml")
                        }
                    }
                }

                Column {
                    id: column3
                    width: 100
                    height: parent.height
                    Item {
                        id: element6
                        width: parent.width
                        height: parent.height
                        Button {
                            id: button1
                            x: 0
                            text: qsTr("Button")
                        }
                    }
                }
            }

            Row {
                id: row4
                width: parent.width
                height: 50
                Item {
                    id: element3
                    width: parent.width
                    height: parent.height

                    Label {
                        id: label1
                        text: qsTr("Subdirectories will be searched")
                        renderType: Text.NativeRendering
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 9
                        anchors.fill: parent
                    }
                }
            }

            Row {
                id: row5
                width: parent.width
                height: 50
                Item {
                    id: element4
                    width: parent.width
                    height: parent.height

                    Label {
                        id: label2
                        text: qsTr("Detection")
                        font.family: "Tahoma"
                        anchors.fill: parent
                        renderType: Text.NativeRendering
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            Row {
                id: row6
                width: parent.width
                height: 50
                Item {
                    id: element5
                    width: parent.width
                    height: parent.height

                    Frame {
                        id: frame1
                        height: 50
                        Column {
                            id: column4
                            anchors.fill: parent
                            Row {
                                id: row17
                                CheckBox {
                                    id: checkBox
                                    width: 125
                                    text: qsTr("Media Players")
                                }

                                CheckBox {
                                    id: checkBox1
                                    text: qsTr("Stream")
                                }
                            }
                        }
                        anchors.fill: parent
                    }
                }
            }


            Row {
                id: row10
                width: parent.width
                height: 50
                Item {
                    id: element8
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label4
                        text: qsTr("Players")
                        font.family: "Tahoma"
                        anchors.fill: parent
                        renderType: Text.NativeRendering
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
            Row {
                id: row9
                x: 0
                width: parent.width
                height: 250

                Item {
                    id: element10
                    width: parent.width
                    height: parent.height

                    Frame {
                        id: frame2
                        x: 0
                        y: 0
                        height: 50
                        Column {
                            id: column5
                            height: 76
                            anchors.fill: parent
                            Row {
                                id: row18
                                CheckBox {
                                    id: checkBox2
                                    width: 125
                                    text: qsTr("Watching")
                                }

                                CheckBox {
                                    id: checkBox3
                                    text: qsTr("Dropped")
                                }

                                CheckBox {
                                    id: checkBox6
                                    text: qsTr("Check Box")
                                }
                            }

                            Row {
                                id: row19
                                CheckBox {
                                    id: checkBox4
                                    width: 125
                                    text: qsTr("Watching")
                                }

                                CheckBox {
                                    id: checkBox5
                                    text: qsTr("Dropped")
                                }

                                CheckBox {
                                    id: checkBox7
                                    text: qsTr("Check Box")
                                }
                            }

                            Row {
                                id: row20
                                CheckBox {
                                    id: checkBox8
                                    width: 125
                                    text: qsTr("Watching")
                                }

                                CheckBox {
                                    id: checkBox9
                                    text: qsTr("Dropped")
                                }

                                CheckBox {
                                    id: checkBox10
                                    text: qsTr("Check Box")
                                }
                            }

                            Row {
                                id: row21
                                CheckBox {
                                    id: checkBox11
                                    width: 125
                                    text: qsTr("Watching")
                                }

                                CheckBox {
                                    id: checkBox12
                                    text: qsTr("Dropped")
                                }

                                CheckBox {
                                    id: checkBox13
                                    text: qsTr("Check Box")
                                }
                            }

                            Row {
                                id: row22
                                CheckBox {
                                    id: checkBox14
                                    width: 125
                                    text: qsTr("Watching")
                                }

                                CheckBox {
                                    id: checkBox15
                                    text: qsTr("Dropped")
                                }

                                CheckBox {
                                    id: checkBox16
                                    text: qsTr("Check Box")
                                }
                            }
                        }
                        anchors.fill: parent
                    }
                }
            }
        }

        Column {
            id: column1
            width: 400
            height: parent.height
            Row {
                id: row8
                width: parent.width
                height: 50
                Item {
                    id: element7
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label3
                        text: qsTr("Use low quality images")
                        font.pointSize: 12
                        anchors.fill: parent
                        renderType: Text.NativeRendering
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                    }
                }
            }

            Row {
                id: row15
                width: parent.width
                height: 50
                Item {
                    id: element13
                    width: parent.width
                    height: parent.height
                    Frame {
                        id: frame3
                        height: 50
                        Column {
                            id: column8
                            anchors.fill: parent
                            Row {
                                id: row23

                                RadioButton {
                                    id: radioButton
                                    text: qsTr("Yes")
                                }

                                RadioButton {
                                    id: radioButton1
                                    text: qsTr("No")
                                }
                            }
                        }
                        anchors.fill: parent
                    }
                }
            }

            Row {
                id: row11
                width: parent.width
                height: 50
                Item {
                    id: element9
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label5
                        text: qsTr("Low quality images are  recommended for those who have slow internet or want things to flow quickly")
                        wrapMode: Text.WordWrap
                        font.family: "Tahoma"
                        font.pointSize: 9
                        anchors.fill: parent
                        renderType: Text.NativeRendering
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                    }
                }
            }
        }
    }
    StackView {
        id: stackViewDiretorios
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.33000001311302185}D{i:8;anchors_height:160;anchors_width:110}
D{i:36;anchors_height:400;anchors_width:200}D{i:35;anchors_height:200;anchors_width:200}
D{i:45;anchors_height:400;anchors_width:200}D{i:44;anchors_height:200;anchors_width:200}
D{i:1;anchors_height:400;anchors_width:200}
}
##^##*/
