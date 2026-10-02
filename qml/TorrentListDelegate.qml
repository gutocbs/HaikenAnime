import QtQuick 2.12
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
                    onCheckStateChanged: mainClass.fchangeTorrentState(index, checkDelegate.checked)
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

                Label {
                    id: nomeAnime
                    color: corFonte
                    text: animeName
                    anchors.fill: parent
                    font.family: "Tahoma"
                    fontSizeMode: Text.Fit
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    leftPadding: 10
                    font.pointSize: 8
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

                Label {
                    id: element3
                    color: corFonte
                    text: animeSub
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    font.family: "Tahoma"
                    fontSizeMode: Text.Fit
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    leftPadding: 10
                    font.pointSize: 8
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
                Label {
                    id: element13
                    color: corFonte
                    text: animeResolution
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    leftPadding: 10
                    font.pointSize: 8
                    fontSizeMode: Text.Fit
                    font.family: "Tahoma"
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

                Label {
                    id: element14
                    color: corFonte
                    text: animeEpisode
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    leftPadding: 10
                    font.pointSize: 8
                    fontSizeMode: Text.Fit
                    font.family: "Tahoma"
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

                Label {
                    id: element15
                    color: corFonte
                    text: animeFileName
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    leftPadding: 10
                    font.pointSize: 8
                    fontSizeMode: Text.Fit
                    font.family: "Tahoma"
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
                Label {
                    id: element16
                    color: corFonte
                    text: animeFileDescription
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    leftPadding: 10
                    font.pointSize: 8
                    fontSizeMode: Text.Fit
                    font.family: "Tahoma"
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
    D{i:0;formeditorZoom:1.100000023841858}
}
##^##*/
