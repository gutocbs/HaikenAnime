import QtQuick 2.0
import QtQuick.Controls 2.3

Item {
    width: 796
    height: 208

    Row {
        id: row2
        anchors.fill: parent

        Column {
            id: column
            width: 310
            height: parent.height

            Row {
                id: row
                width: 310
                height: 150

                AnimeReconhecidoSlot {
                    id: animeReconhecidoSlot
                }
            }

        }

        Column {
            id: column1
            width: parent.width-column.width
            height: 40

            Row {
                id: row1
                width: parent.width
                height: 60
                spacing: 0


                Row {
                    id: row4
                    width: parent.width-100
                    height: parent.height
                    layoutDirection: Qt.RightToLeft
                    spacing: 3

                    Button {
                        id: type
                        width: 60
                        height: 60
                        text: qsTr("Type")
                        onClicked: mainClass.fordemLista("formato")
                    }

                    Button {
                        id: release
                        width: 60
                        height: 60
                        text: qsTr("Release")
                        onClicked: mainClass.fordemLista("data")
                    }

                    Button {
                        id: score
                        width: 60
                        height: 60
                        text: qsTr("Score")
                        onClicked: mainClass.fordemLista("nota")
                    }

                    Button {
                        id: progress
                        width: 60
                        height: 60
                        text: qsTr("Progress")
                        onClicked: mainClass.fordemLista("progresso")
                    }

                    Button {
                        id: alphabetical
                        width: 60
                        height: 60
                        text: qsTr("Alphabetical")
                        onClicked: mainClass.fordemLista("nome")
                    }
                }

                Row {
                    id: row5
                    x: parent.width - 100
                    width: 100
                    height: parent.height
                    layoutDirection: Qt.RightToLeft

                    Button {
                        id: foward
                        width: 40
                        text: qsTr("Pra frente")
                        onClicked: mainClass.fproximaPagina()
                    }

                    Button {
                        id: backward
                        width: 40
                        text: qsTr("Pra trás")
                        onClicked: mainClass.fanteriorPagina()
                    }

                }
            }

            Row {
                id: row3
                width: parent.width
                height: 100
            }

        }



    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:2}D{i:4;anchors_height:400;anchors_width:200}D{i:2;anchors_width:260}
D{i:8;anchors_width:60}D{i:1;anchors_height:400;anchors_width:200}
}
##^##*/
