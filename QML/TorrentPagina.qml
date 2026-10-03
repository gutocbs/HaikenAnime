import QtQuick 2.12
import QtQuick.Controls 2.3

Item {
    id: abaTorrent
    width: 1920
    height: 814
    scale: 1

    property bool leuLista: false
    property bool ordemInversa: false

    Component.onCompleted: mainClass.fsetTorrentList()
    Connections{
        target: mainClass
        onStorrentPronto: {
            var lista = mainClass.fgetTorrentList("prioridade")
            modeloListaTorrent.clear()
            row2.visible = true
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
            leftPadding: 8

            Column {
                id: column1
                width: 110
                height: parent.height

                Item {
                    id: element
                    width: parent.width
                    height: parent.height

                    Button {
                        id: botaoUpdate
                        text: qsTr("Update")
                        onClicked: {
                            if(modeloListaTorrent.count != 0)
                                modeloListaTorrent.clear()
                            mainClass.fsetTorrentList()
                        }
                    }
                }
            }

            Column {
                id: column9
                width: 110
                height: parent.height
                Item {
                    id: element5
                    width: parent.width
                    height: parent.height
                    Button {
                        id: botaoDownload
                        text: qsTr("Download")
                        onClicked: {
                            mainClass.fbotaoDownloadTorrents()
                            if(modeloListaTorrent.count != 0)
                                modeloListaTorrent.clear()
                            mainClass.fsetTorrentList()
                        }
                    }
                }
            }

            Column {
                id: column10
                width: 110
                height: parent.height
                Item {
                    id: element7
                    width: parent.width
                    height: parent.height
                    Button {
                        id: botaoSelectSub
                        text: qsTr("Select Sub")
                    }
                }
            }

            Column {
                id: column11
                width: 110
                height: parent.height
                Item {
                    id: element9
                    width: parent.width
                    height: parent.height
                    Button {
                        id: botaoAnimeInfo
                        text: qsTr("Anime Info")
                    }
                }
            }

            Column {
                id: column12
                width: 110
                height: parent.height
                Item {
                    id: element11
                    width: parent.width
                    height: parent.height
                    Button {
                        id: botaoOpenLink
                        text: qsTr("Open Link")
                        onClicked: Qt.openUrlExternally(mainClass.fopenTorrentLink(listaTorrents.currentIndex))
                    }
                }
            }

            Column {
                id: column13
                width: 110
                height: parent.height
                Item {
                    id: element13
                    width: parent.width
                    height: parent.height
                    Button {
                        id: botaoSearchAnime
                        text: qsTr("Search for this anime")
                        onClicked: mainClass.fsearchAnimeFromTorrent(listaTorrents.currentIndex)
                    }
                }
            }
        }


        Row {
            id: row2
            width: parent.width
            height: 30
            visible: false


            Column {
                id: column14
                width: parent.width
                height: parent.height

                Row {
                    id: row3
                    x: 0
                    width: parent.width
                    height: parent.height
                    Column {
                        id: column2
                        width: 50
                        height: parent.height
                        Item {
                            id: element2
                            width: parent.width
                            height: parent.height
                            Rectangle {
                                id: rectangle
                                color: "#ffffff"
                                visible: true
                                anchors.fill: parent
                                border.color: "#d8d8d8"
                                border.width: 1
                            }

                            MouseArea {
                                id: mouseAreaCheckbox
                                anchors.fill: parent
                                onClicked: {
                                    var lista
                                    if(!ordemInversa){
                                        lista = mainClass.fgetTorrentList("ccheck")
                                        ordemInversa = true
                                    }
                                    else{
                                        lista = mainClass.fgetTorrentList("dcheck")
                                        ordemInversa = false
                                    }
                                    modeloListaTorrent.clear()
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
                        }
                    }

                    Column {
                        id: column3
                        width: 470
                        height: parent.height
                        Item {
                            id: element4
                            width: parent.width
                            height: parent.height

                            Rectangle {
                                id: rectangle1
                                color: "#ffffff"
                                anchors.fill: parent
                                border.color: "#d8d8d8"
                                border.width: 1
                            }

                            MouseArea {
                                id: mouseAreaAnimeName
                                anchors.fill: parent
                                onClicked: {
                                    var lista
                                    if(!ordemInversa){
                                        lista = mainClass.fgetTorrentList("cnomeAnime")
                                        ordemInversa = true
                                    }
                                    else{
                                        lista = mainClass.fgetTorrentList("dnomeAnime")
                                        ordemInversa = false
                                    }
                                    modeloListaTorrent.clear()
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
                            Text {
                                id: labelAnimeName
                                text: "Anime Name"
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                anchors.fill: parent
                                font.family: "Tahoma"
                                verticalAlignment: Text.AlignVCenter
                                fontSizeMode: Text.Fit
                                wrapMode: Text.WordWrap
                                font.pixelSize: 15
                                anchors.leftMargin: 5
                                renderType: Text.NativeRendering
                            }
                        }
                    }

                    Column {
                        id: column4
                        width: 100
                        height: parent.height
                        Item {
                            id: element6
                            width: parent.width
                            height: parent.height


                            MouseArea {
                                id: mouseAreaSubname
                                anchors.fill: parent
                                onClicked: {
                                    var lista
                                    if(!ordemInversa){
                                        lista = mainClass.fgetTorrentList("cnomeSub")
                                        ordemInversa = true
                                    }
                                    else{
                                        lista = mainClass.fgetTorrentList("dnomeSub")
                                        ordemInversa = false
                                    }
                                    modeloListaTorrent.clear()
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
                            Rectangle {
                                id: rectangle2la
                                color: "#ffffff"
                                anchors.fill: parent
                                border.color: "#d8d8d8"
                                border.width: 1
                            }
                            Text {
                                id: labelSubName
                                text: "Sub Name"
                                font.bold: true
                                anchors.fill: parent
                                font.family: "Tahoma"
                                verticalAlignment: Text.AlignVCenter
                                fontSizeMode: Text.Fit
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                                font.pixelSize: 15
                                renderType: Text.NativeRendering
                            }
                        }
                    }

                    Column {
                        id: column5
                        width: 100
                        height: parent.height
                        Item {
                            id: element8
                            width: parent.width
                            height: parent.height


                            MouseArea {
                                id: mouseAreaResolutuion
                                anchors.fill: parent
                                onClicked: {
                                    var lista
                                    if(!ordemInversa){
                                        lista = mainClass.fgetTorrentList("cresolucao")
                                        ordemInversa = true
                                    }
                                    else{
                                        lista = mainClass.fgetTorrentList("dresolucao")
                                        ordemInversa = false
                                    }
                                    modeloListaTorrent.clear()
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
                            Rectangle {
                                id: rectangle3
                                color: "#ffffff"
                                anchors.fill: parent
                                border.color: "#d8d8d8"
                                border.width: 1
                            }
                            Text {
                                id: labelResolution
                                text: "Resolution"
                                font.bold: true
                                anchors.fill: parent
                                font.family: "Tahoma"
                                verticalAlignment: Text.AlignVCenter
                                fontSizeMode: Text.Fit
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                                font.pixelSize: 15
                                renderType: Text.NativeRendering
                            }
                        }
                    }

                    Column {
                        id: column6
                        width: 100
                        height: parent.height
                        Item {
                            id: element10
                            width: parent.width
                            height: parent.height


                            MouseArea {
                                id: mouseAreaEpisode
                                anchors.fill: parent
                                onClicked: {
                                    var lista
                                    if(!ordemInversa){
                                        lista = mainClass.fgetTorrentList("cepisodio")
                                        ordemInversa = true
                                    }
                                    else{
                                        lista = mainClass.fgetTorrentList("depisodio")
                                        ordemInversa = false
                                    }
                                    modeloListaTorrent.clear()
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
                            Rectangle {
                                id: rectangle6
                                color: "#ffffff"
                                anchors.fill: parent
                                border.color: "#d8d8d8"
                                border.width: 1
                            }
                            Text {
                                id: labelEpisode
                                text: "Episode"
                                font.bold: true
                                anchors.fill: parent
                                font.family: "Tahoma"
                                verticalAlignment: Text.AlignVCenter
                                fontSizeMode: Text.Fit
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                                font.pixelSize: 15
                                renderType: Text.NativeRendering
                            }
                        }
                    }

                    Column {
                        id: column7
                        width: 400
                        height: parent.height
                        Item {
                            id: element12
                            width: parent.width
                            height: parent.height


                            MouseArea {
                                id: mouseAreaFileName
                                anchors.fill: parent
                                onClicked: {
                                    var lista
                                    if(!ordemInversa){
                                        lista = mainClass.fgetTorrentList("cnomeArquivo")
                                        ordemInversa = true
                                    }
                                    else{
                                        lista = mainClass.fgetTorrentList("dnomeArquivo")
                                        ordemInversa = false
                                    }
                                    modeloListaTorrent.clear()
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
                            Rectangle {
                                id: rectangle4
                                color: "#ffffff"
                                anchors.fill: parent
                                anchors.leftMargin: 0
                                border.color: "#d8d8d8"
                                border.width: 1
                            }
                            Text {
                                id: labelFileName
                                text: "File Name"
                                font.bold: true
                                anchors.fill: parent
                                font.family: "Tahoma"
                                verticalAlignment: Text.AlignVCenter
                                fontSizeMode: Text.Fit
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                                font.pixelSize: 15
                                anchors.leftMargin: 5
                                renderType: Text.NativeRendering
                            }
                        }
                    }

                    Column {
                        id: column8
                        width: parent.width - column2.width - column3.width - column6.width - column5.width - column6.width - column7.width
                        height: parent.height
                        Item {
                            id: element14
                            width: parent.width
                            height: parent.height


                            MouseArea {
                                id: mouseAreaDescription
                                anchors.fill: parent
                                onClicked: {
                                    var lista
                                    if(!ordemInversa){
                                        lista = mainClass.fgetTorrentList("cdescricao")
                                        ordemInversa = true
                                    }
                                    else{
                                        lista = mainClass.fgetTorrentList("ddescricao")
                                        ordemInversa = false
                                    }
                                    modeloListaTorrent.clear()
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
                            Rectangle {
                                id: rectangle5
                                color: "#ffffff"
                                visible: true
                                anchors.fill: parent
                                anchors.leftMargin: 0
                                border.color: "#d8d8d8"
                                border.width: 1
                            }
                            Text {
                                id: labelDescription
                                color: "#000000"
                                text: "Description"
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                anchors.fill: parent
                                font.family: "Tahoma"
                                verticalAlignment: Text.AlignVCenter
                                fontSizeMode: Text.Fit
                                wrapMode: Text.WordWrap
                                font.pixelSize: 15
                                anchors.leftMargin: 5
                                renderType: Text.NativeRendering
                            }
                        }
                    }
                }
            }
        }
        Row {
            id: row1
            width: parent.width
            height: parent.height-row.height-row2.height

            Item {
                id: element1
                width: parent.width
                height: parent.height

                ListView {
                    id: listaTorrents
                    x: 0
                    y: 0
                    anchors.rightMargin: 0
                    anchors.bottomMargin: 0
                    anchors.leftMargin: 0
                    anchors.topMargin: 0
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
    D{i:0;formeditorZoom:0.75}D{i:1;anchors_height:400;anchors_width:200}D{i:4;anchors_height:208;anchors_width:834;anchors_y:606}
D{i:9;anchors_height:200;anchors_width:200;anchors_x:0;anchors_y:0}D{i:8;anchors_height:200;anchors_width:200}
D{i:7;anchors_height:208;anchors_width:834;anchors_y:606}D{i:10;anchors_height:208;anchors_width:834;anchors_y:606}
D{i:14;anchors_height:100;anchors_width:100}D{i:13;anchors_height:208;anchors_width:834;anchors_y:606}
D{i:18;anchors_height:100;anchors_width:100}D{i:16;anchors_height:208;anchors_width:834;anchors_y:606}
D{i:19;anchors_height:208;anchors_width:834;anchors_y:606}D{i:3;anchors_height:814;anchors_width:834;anchors_x:0;anchors_y:0}
D{i:22;anchors_height:200;anchors_width:200}D{i:61;anchors_height:200;anchors_width:200}
D{i:60;anchors_height:200;anchors_width:200}D{i:2;anchors_height:814;anchors_width:834;anchors_x:0;anchors_y:0}
}
##^##*/
