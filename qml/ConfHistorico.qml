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
                        id: listaHistorico
                        x: 0
                        y: 0
                        flickableDirection: Flickable.AutoFlickDirection
                        clip: true
                        highlightRangeMode: ListView.NoHighlightRange
                        anchors.fill: parent
                        model: ListModel {
                            id: modeloLista
                        }
                        delegate: Item {
                            x: 5
                            id: itemDir
                            width: listaHistorico.width
                            height: 40
                            Row {
                                id: row7
                                width: parent.width
                                height: parent.height
                                Item{
                                    width: parent.width
                                    height: parent.height
                                    Text {
                                        text: name
                                        anchors.fill: parent
                                        verticalAlignment: Text.AlignVCenter
                                        font.bold: true
                                    }
                                }

                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: listaHistorico.currentIndex = index
                            }
                        }
                        highlight: Rectangle {
                            id: rectangleHighlight
                            color: "#0da2ff"
                            opacity: 0.5
                            radius: 0
                        }
                        focus: true
                        Component.onCompleted: {
                            var diretorios = mainClass.fgetDir();
                            for(var i = 0; i < diretorios.length; i++){
//                                modeloLista.append({"name": diretorios[i].toString()})
                                modeloLista.append({"name": "anime " + i})
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
