import QtQuick 2.12
import QtQuick.Controls 2.3

Item {
    id: abaTorrent
    width: 1920
    height: 814
    scale: 1
    property bool leuLista: false
    Component.onCompleted: mainClass.fsetTorrentList()
    Connections{
        target: mainClass
        onStorrentPronto: {
            var lista = mainClass.fgetTorrentList()
            for(var i = 0; i < lista.length; i++){
                if(lista[i][0] === "true"){
                    modeloListaTorrent.append({"check": true, "animeName": lista[i][1],
                                               "animeSub": lista[i][2], "animeResolution": lista[i][3],
                                               "animeEpisode": lista[i][4], "animeFileName": lista[i][5],
                                               "animeFileDescription": lista[i][6], "corFonte": "royalblue"})
                }
                else{
                    modeloListaTorrent.append({"check": false, "animeName": lista[i][1],
                                               "animeSub": lista[i][2], "animeResolution": lista[i][3],
                                               "animeEpisode": lista[i][4], "animeFileName": lista[i][5],
                                               "animeFileDescription": lista[i][6], "corFonte": "black"})
                }
            }
        }
    }
    Column {
        id: column
        anchors.fill: parent

        Row {
            id: row
            width: parent.width
            height: 100

            Column {
                id: column1
                width: 200
                height: parent.height

                Item {
                    id: element
                    width: parent.width
                    height: parent.height

                    Button {
                        id: botaoRefresh
                        text: qsTr("Refresh")
                        onClicked: {
                            if(modeloListaTorrent.count != 0)
                                modeloListaTorrent.clear()
                            mainClass.fsetTorrentList()
                        }
                    }
                }
            }
        }

        Row {
            id: row1
            width: parent.width
            height: parent.height-row.height

            Item {
                id: element1
                width: parent.width
                height: parent.height

                ListView {
                    id: listaTorrents
                    x: 0
                    y: 0
                    boundsMovement: Flickable.StopAtBounds
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true
                    flickableDirection: Flickable.AutoFlickDirection
                    highlightRangeMode: ListView.NoHighlightRange
                    highlightMoveDuration: 1
                    highlightMoveVelocity : 1000
                    ScrollBar.vertical: ScrollBar {
                        active: true
                        policy: ScrollBar.AlwaysOn
                        contentItem: Rectangle{
                            implicitWidth: 6
                            implicitHeight: 100
                            radius: width / 2
                            color: "#454545"//control.pressed ? "#454545" : "#050505"
                        }
                    }
                    anchors.fill: parent
                    model: ListModel {
                        id: modeloListaTorrent
                    }
                    delegate: TorrentListDelegate{}
                    highlight: Rectangle {
                        id: rectangleHighlight
                        color: "#0da2ff"
                        opacity: 0.5
                        radius: 0
                        z: Infinity
                    }
                    focus: true
                }
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.33000001311302185}D{i:4;anchors_height:208;anchors_width:834;anchors_y:606}
D{i:3;anchors_height:814;anchors_width:834;anchors_x:0;anchors_y:0}D{i:2;anchors_height:814;anchors_width:834;anchors_x:0;anchors_y:0}
D{i:1;anchors_height:400;anchors_width:200}
}
##^##*/
