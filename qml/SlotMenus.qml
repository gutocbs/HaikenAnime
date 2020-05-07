import QtQuick 2.0
import QtQuick.Controls 2.3

Item {
    width: 1920
    height: 187

    Row {
        id: row
        anchors.bottomMargin: 20
        anchors.topMargin: 20
        anchors.fill: parent

        Column {
            id: column
            width: 700
            height: parent.height

            Row {
                id: row1
                width: 500
                height: parent.height/3
                spacing: 2

                Button {
                    id: button
                    text: qsTr("Anime")
                    display: AbstractButton.TextBesideIcon
                    font.family: "Tahoma"
                }

                Button {
                    id: button1
                    text: qsTr("Manga")
                    font.family: "Tahoma"
                }

                Button {
                    id: button2
                    text: qsTr("LN")
                    font.family: "Tahoma"
                }

                Button {
                    id: button3
                    text: qsTr("Season")
                    font.family: "Tahoma"
                }

                Item {
                    id: element
                    width: 400
                    height: row1.height+row2.height

                    Row {
                        id: row4
                        spacing: 9
                        anchors.fill: parent

                        Image {
                            id: image
                            width: 100
                            height: 100
                            fillMode: Image.PreserveAspectFit
                            source: "qrc:/qtquickplugin/images/template_image.png"
                        }

                        Label {
                            id: label
                            width: 200
                            height: 50
                            text: qsTr("Label")
                            font.bold: true
                            font.pointSize: 24
                            font.family: "Tahoma"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }

            Row {
                id: row2
                width: 200
                height: parent.height/3
                spacing: 2

                Button {
                    id: button4
                    text: qsTr("Home")
                    font.family: "Tahoma"
                }

                Button {
                    id: button5
                    text: qsTr("Configuration")
                    font.family: "Tahoma"
                }

                Button {
                    id: button6
                    text: qsTr("Torrent")
                    font.family: "Tahoma"
                }

                Button {
                    id: button7
                    text: qsTr("Help")
                    font.family: "Tahoma"
                }
            }

            Row {
                id: row3
                width: 200
                height: parent.height/3
                spacing: 2

                Button {
                    id: botaoWatching
                    text: qsTr("Watching")
                    font.family: "Tahoma"
                    onClicked: mainClass.fselecionaListaCurrent()
                }

                Button {
                    id: button9
                    text: qsTr("Completed")
                    font.family: "Tahoma"
                    onClicked: mainClass.fselecionaListaCompleted()
                }

                Button {
                    id: button10
                    text: qsTr("On Hold")
                    font.family: "Tahoma"
                    onClicked: mainClass.fselecionaListaPaused()
                }

                Button {
                    id: button11
                    text: qsTr("Dropped")
                    font.family: "Tahoma"
                    onClicked: mainClass.fselecionaListaDropped()
                }

                Button {
                    id: button12
                    text: qsTr("Plan to Watch")
                    font.family: "Tahoma"
                    onClicked: mainClass.fselecionaListaPlanning()
                }

                Button {
                    id: button13
                    text: qsTr("Refresh")
                    font.family: "Tahoma"
                }
            }
        }
    }

}

/*##^##
Designer {
    D{i:0;formeditorZoom:4}D{i:9;anchors_height:400}D{i:1;anchors_height:147;anchors_width:200}
}
##^##*/
