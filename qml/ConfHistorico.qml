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
                        text: qsTr("History")
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
                width: parent.width
                height: parent.height - row1.height
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
                        focus: true
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
                        highlight: Rectangle {
                            color: "#ff0000"
                            radius: 20
                        }
                        highlightRangeMode: ListView.NoHighlightRange
                        anchors.fill: parent
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
                    }
                }
            }
        }
    }

}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.8999999761581421}D{i:8;anchors_height:200;anchors_width:200}
D{i:9;anchors_height:160;anchors_width:110}D{i:2;anchors_height:400;anchors_width:200}
D{i:1;anchors_height:400;anchors_width:200}
}
##^##*/
