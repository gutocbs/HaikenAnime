import QtQuick 2.0
import QtQuick.Controls 2.3

Item {
    id: delegateTorrent
    width: 1920
    height: 40
//    property bool check: false
    MouseArea {
        anchors.fill: parent
        onClicked: listaTorrents.currentIndex = index
    }


    Rectangle {
        id: rectangle7
        color: "#ffffff"
        anchors.fill: parent
    }

    Row {
        id: row
        anchors.fill: parent

        Column {
            id: column
            width: 50
            height: parent.height

            Item {
                id: element
                width: parent.width
                height: parent.height


                Rectangle {
                    id: rectangle
                    color: "#00000000"
                    border.color: "#d8d8d8"
                    border.width: 1
                    anchors.fill: parent
                }
                CheckDelegate {
                    id: checkDelegate
                    width: parent.width/4
                    height: parent.height/4
                    display: AbstractButton.IconOnly
                    anchors.fill: parent
                    checked: check
                    onCheckStateChanged: mainClass.fchangeTorrentState(index)
                }
            }
        }

        Column {
            id: column1
            width: 470
            height: parent.height
            Item {
                id: element1
                width: parent.width
                height: parent.height

                Text {
                    id: element2
                    color: corFonte
                    text: animeName
                    anchors.leftMargin: 5
                    font.family: "Tahoma"
                    fontSizeMode: Text.Fit
                    renderType: Text.NativeRendering
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    anchors.fill: parent
                    font.pixelSize: 12
                }

                Rectangle {
                    id: rectangle1
                    color: "#00000000"
                    anchors.fill: parent
                    border.width: 1
                    border.color: "#d8d8d8"
                }
            }
        }

        Column {
            id: column2
            width: 100
            height: parent.height
            Item {
                id: element4
                width: parent.width
                height: parent.height
                Text {
                    id: element3
                    text: animeSub
                    color: corFonte
                    horizontalAlignment: Text.AlignHCenter
                    renderType: Text.NativeRendering
                    anchors.fill: parent
                    fontSizeMode: Text.Fit
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    verticalAlignment: Text.AlignVCenter
                    font.family: "Tahoma"
                }

                Rectangle {
                    id: rectangle2
                    color: "#00000000"
                    anchors.fill: parent
                    border.width: 1
                    border.color: "#d8d8d8"
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
                Text {
                    id: element5
                    text: animeResolution
                    color: corFonte
                    horizontalAlignment: Text.AlignHCenter
                    renderType: Text.NativeRendering
                    anchors.fill: parent
                    fontSizeMode: Text.Fit
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    font.family: "Tahoma"
                    verticalAlignment: Text.AlignVCenter
                }

                Rectangle {
                    id: rectangle3
                    color: "#00000000"
                    anchors.fill: parent
                    border.width: 1
                    border.color: "#d8d8d8"
                }
            }
        }

        Column {
            id: column4
            width: 100
            height: parent.height
            Item {
                id: element8
                width: parent.width
                height: parent.height
                Text {
                    id: element7
                    text: animeEpisode
                    color: corFonte
                    horizontalAlignment: Text.AlignHCenter
                    renderType: Text.NativeRendering
                    anchors.fill: parent
                    fontSizeMode: Text.Fit
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    font.family: "Tahoma"
                    verticalAlignment: Text.AlignVCenter
                }

                Rectangle {
                    id: rectangle6
                    color: "#00000000"
                    anchors.fill: parent
                    border.width: 1
                    border.color: "#d8d8d8"
                }
            }
        }

        Column {
            id: column5
            width: 400
            height: parent.height
            Item {
                id: element10
                width: parent.width
                height: parent.height
                Text {
                    id: element9
                    text: animeFileName
                    color: corFonte
                    anchors.leftMargin: 5
                    horizontalAlignment: Text.AlignLeft
                    renderType: Text.NativeRendering
                    anchors.fill: parent
                    fontSizeMode: Text.Fit
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    font.family: "Tahoma"
                    verticalAlignment: Text.AlignVCenter
                }

                Rectangle {
                    id: rectangle4
                    color: "#00000000"
                    anchors.leftMargin: 0
                    anchors.fill: parent
                    border.width: 1
                    border.color: "#d8d8d8"
                }
            }
        }

        Column {
            id: column6
            width: parent.width - column.width - column1.width - column2.width - column3.width - column4.width - column5.width
            height: parent.height
            Item {
                id: element12
                width: parent.width
                height: parent.height
                Text {
                    id: element11
                    text: animeFileDescription
                    color: corFonte
                    anchors.leftMargin: 5
                    renderType: Text.NativeRendering
                    anchors.fill: parent
                    fontSizeMode: Text.Fit
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    font.family: "Tahoma"
                    verticalAlignment: Text.AlignVCenter
                }

                Rectangle {
                    id: rectangle5
                    color: "#00000000"
                    anchors.leftMargin: 0
                    anchors.fill: parent
                    border.width: 1
                    border.color: "#d8d8d8"
                }
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.75}D{i:2;anchors_height:200;anchors_width:200}D{i:6;anchors_height:200;anchors_width:200}
D{i:11;anchors_height:200;anchors_width:200}D{i:15;anchors_height:200;anchors_width:200}
D{i:19;anchors_height:200;anchors_width:200}D{i:23;anchors_height:200;anchors_width:200}
D{i:27;anchors_height:200;anchors_width:200}D{i:31;anchors_height:200;anchors_width:200}
}
##^##*/
