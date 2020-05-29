import QtQuick 2.0
import QtQuick.Controls 2.3

Item {
    width: 1020
    height: 814

    Row {
        id: row14
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
                        id: label1
                        text: qsTr("Torrent Configuration")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: true
                        font.pointSize: 14
                    }
                }
            }

            Row {
                id: row2
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element1
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label2
                        text: qsTr("Preferred Torrent")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }

            Row {
                id: row3
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element2
                    width: parent.width
                    height: parent.height

                    ComboBox {
                        id: comboBox
                    }
                }
            }

            Row {
                id: row4
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element3
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label3
                        text: qsTr("Download torrents automatically")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }

            Row {
                id: row5
                x: 0
                width: parent.width
                height: 50

                Item {
                    id: element17
                    width: parent.width
                    height: parent.height

                    Frame {
                        id: frame
                        x: 0
                        y: 0
                        anchors.fill: parent

                        Row {
                            id: row
                            anchors.fill: parent

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
                }
            }

            Row {
                id: row6
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element5
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label5
                        text: qsTr("Download torrent after")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }

            Row {
                id: row7
                x: 0
                width: parent.width
                height: 50

                Column {
                    id: column1
                    width: 100
                    height: parent.height

                    Item {
                        id: element4
                        width: parent.width
                        height: parent.height

                        TextField {
                            id: textField
                            text: qsTr("10")
                            horizontalAlignment: Text.AlignHCenter
                            placeholderText: "10"
                            anchors.fill: parent
                        }
                    }
                }

                Column {
                    id: column2
                    width: 100
                    height: parent.height
                    Item {
                        id: element6
                        width: parent.width
                        height: parent.height

                        Label {
                            id: label
                            text: qsTr("minutes")
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignLeft
                            anchors.fill: parent
                        }
                    }
                }
            }

            Row {
                id: row8
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element7
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label7
                        text: qsTr("Download animes from lists")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }

            Row {
                id: row9
                x: 0
                width: parent.width
                height: 100

                Item {
                    id: element8
                    width: parent.width
                    height: parent.height

                    Frame {
                        id: frame1
                        x: 0
                        y: 0
                        height: 50
                        anchors.fill: parent

                        Column {
                            id: column3
                            anchors.fill: parent

                            Row {
                                id: row17

                                CheckBox {
                                    id: checkBox
                                    width: 125
                                    text: qsTr("Watching")
                                }

                                CheckBox {
                                    id: checkBox1
                                    text: qsTr("Dropped")
                                }
                            }

                            Row {
                                id: row18
                                CheckBox {
                                    id: checkBox2
                                    text: qsTr("Plan to Watch")
                                }

                                CheckBox {
                                    id: checkBox3
                                    text: qsTr("On Hold")
                                }
                            }
                        }

                    }
                }
            }

            Row {
                id: row10
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element9
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label9
                        text: qsTr("Preferred sub")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }

            Row {
                id: row11
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element10
                    width: parent.width
                    height: parent.height

                    TextField {
                        id: textField1
                        height: parent.height
                        text: qsTr("10")
                        placeholderText: "10"
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            Row {
                id: row12
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element11
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label11
                        text: qsTr("Prefered quality")
                        anchors.rightMargin: 0
                        anchors.bottomMargin: 0
                        anchors.leftMargin: 0
                        anchors.topMargin: 0
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }

            Row {
                id: row13
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element12
                    width: parent.width
                    height: parent.height

                    ComboBox {
                        id: comboBox1
                    }
                }
            }
        }

        Column {
            id: column4
            width: 400
            height: parent.height

            Row {
                id: row15
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element13
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label10
                        text: qsTr("General anime torrent feed")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }

            Row {
                id: row16
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element14
                    width: parent.width
                    height: parent.height
                    TextField {
                        id: textField2
                        text: qsTr("10")
                        anchors.fill: parent
                        placeholderText: "10"
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            Row {
                id: row19
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element15
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label12
                        text: qsTr("Specific anime torrent feed")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }

            Row {
                id: row20
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element16
                    width: parent.width
                    height: parent.height
                    TextField {
                        id: textField3
                        text: qsTr("10")
                        anchors.fill: parent
                        placeholderText: "10"
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }
    }

}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.33000001311302185}D{i:17;anchors_height:400;anchors_width:200}
D{i:16;anchors_height:200;anchors_width:200}D{i:35;anchors_height:400;anchors_width:200}
D{i:34;anchors_height:200;anchors_width:200}D{i:2;anchors_height:400;anchors_width:200}
D{i:60;anchors_height:50}D{i:66;anchors_height:50}D{i:1;anchors_height:400;anchors_width:200}
}
##^##*/
