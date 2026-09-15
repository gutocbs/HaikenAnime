import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Dialogs 1.3

Item {
    id: configDir
    width: 1020
    height: 814

    property bool lowQuality: false
    property var detection: []
    property var players: []

    Component.onCompleted: {
        if(!configDiretorio.lowQuality){
            qualityFalse.checked = true
        }
        else{
            qualityTrue.checked = true
        }
        players = configDiretorio.players.toString().split(",")
        detection = configDiretorio.detection.toString().split(",")
    }

    function fcheckState(type, name){
        if(type === "detectionType" && detection.includes(name))
            return true
        else if(type === "mediaPlayers" && players.includes(name))
            return true
        return false
    }

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
                            width: listaDiretorios.width
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
                                onClicked: listaDiretorios.currentIndex = index
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
                                modeloLista.append({"name": diretorios[i].toString(), "colorCode": "black"})
                            }
                        }
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
                            text: qsTr("Remove")
                            onClicked: {
                                if(mainClass.fremoveDirectory(modeloLista.get(listaDiretorios.currentIndex).name))
                                    modeloLista.remove(listaDiretorios.currentIndex)
                            }
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
                        id: detectionType
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

                                    Component.onCompleted:{
                                        if(fcheckState("detectionType", "MP"))
                                            checked = true
                                    }

                                    onCheckStateChanged: {
                                        if(detection.includes("MP") && checkState === 0)
                                            detection.splice(detection.indexOf("MP"), 1)
                                        else if(checkState === 2 && !detection.includes("MP"))
                                            detection.unshift("MP")
                                        configDiretorio.detection = detection
                                    }
                                }
                                CheckBox {
                                    id: checkBox1
                                    text: qsTr("Stream")
                                    Component.onCompleted:{
                                        if(fcheckState("detectionType", "ST"))
                                            checked = true
                                    }
                                    onCheckStateChanged: {
                                        if(detection.includes("ST") && checkState === 0)
                                            detection.splice(detection.indexOf("ST"), 1)
                                        else if(checkState === 2 && !detection.includes("ST"))
                                            detection.unshift("ST")
                                        configDiretorio.detection = detection
                                    }
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
                        id: mediaPlayers
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
                                    text: qsTr("Crunchyroll")
                                    Component.onCompleted:{
                                        if(fcheckState("mediaPlayers", text.replace(" ","").toUpperCase()))
                                            checked = true
                                    }
                                    onCheckStateChanged: {
                                        if(players.includes("CRUNCHYROLL") && checkState === 0)
                                            players.splice(players.indexOf("CRUNCHYROLL"), 1)
                                        else if(checkState === 2 && !detection.includes("CRUNCHYROLL"))
                                            players.unshift("CRUNCHYROLL")
                                        configDiretorio.players = players
                                    }
                                }

                                CheckBox {
                                    id: checkBox3
                                    width: 165
                                    text: qsTr("Baka MPlayer")
                                    Component.onCompleted:{
                                        if(fcheckState("mediaPlayers", text.replace(" ","").toUpperCase()))
                                            checked = true
                                    }
                                    onCheckStateChanged: {
                                        if(players.includes("BAKAMPLAYER") && checkState === 0)
                                            players.splice(players.indexOf("BAKAMPLAYER"), 1)
                                        else if(checkState === 2)
                                            players.unshift("BAKAMPLAYER" && !detection.includes("BAKAMPLAYER"))
                                        configDiretorio.players = players
                                    }
                                }

                                CheckBox {
                                    id: checkBox6
                                    text: qsTr("Windows Media Player")
                                    Component.onCompleted:{
                                        if(fcheckState("mediaPlayers", text.replace(" ","").toUpperCase()))
                                            checked = true
                                    }
                                    onCheckStateChanged: {
                                        if(players.includes("WINDOWSMEDIAPLAYER") && checkState === 0)
                                            players.splice(players.indexOf("WINDOWSMEDIAPLAYER"), 1)
                                        else if(checkState === 2 && !detection.includes("WINDOWSMEDIAPLAYER"))
                                            players.unshift("WINDOWSMEDIAPLAYER")
                                        configDiretorio.players = players
                                    }
                                }
                            }

                            Row {
                                id: row19
                                CheckBox {
                                    id: checkBox4
                                    width: 125
                                    text: qsTr("AnimeLab")
                                    Component.onCompleted:{
                                        if(fcheckState("mediaPlayers", text.replace(" ","").toUpperCase()))
                                            checked = true
                                    }
                                    onCheckStateChanged: {
                                        if(players.includes("ANIMELAB") && checkState === 0)
                                            players.splice(players.indexOf("ANIMELAB"), 1)
                                        else if(checkState === 2 && !detection.includes("WINDOWSMEDIAPLAYER"))
                                            players.unshift("ANIMELAB")
                                        configDiretorio.players = players
                                    }
                                }

                                CheckBox {
                                    id: checkBox5
                                    width: 165
                                    text: qsTr("Media Player Classic")
                                    Component.onCompleted:{
                                        if(fcheckState("mediaPlayers", text.replace(" ","").toUpperCase()))
                                            checked = true
                                    }
                                    onCheckStateChanged: {
                                        if(players.includes("MEDIAPLAYERCLASSIC") && checkState === 0)
                                            players.splice(players.indexOf("MEDIAPLAYERCLASSIC"), 1)
                                        else if(checkState === 2 && !detection.includes("WINDOWSMEDIAPLAYER"))
                                            players.unshift("MEDIAPLAYERCLASSIC")
                                        configDiretorio.players = players
                                    }
                                }

                                CheckBox {
                                    id: checkBox7
                                    text: qsTr("WebTorrent")
                                    Component.onCompleted:{
                                        if(fcheckState("mediaPlayers", text.replace(" ","").toUpperCase()))
                                            checked = true
                                    }
                                    onCheckStateChanged: {
                                        if(players.includes("WEBTORRENT") && checkState === 0)
                                            players.splice(players.indexOf("WEBTORRENT"), 1)
                                        else if(checkState === 2 && !detection.includes("WEBTORRENT"))
                                            players.unshift("WEBTORRENT")
                                        configDiretorio.players = players
                                    }
                                }
                            }

                            Row {
                                id: row20
                                CheckBox {
                                    id: checkBox8
                                    width: 125
                                    text: qsTr("Funimation")
                                    Component.onCompleted:{
                                        if(fcheckState("mediaPlayers", text.replace(" ","").toUpperCase()))
                                            checked = true
                                    }
                                    onCheckStateChanged: {
                                        if(players.includes("FUNIMATION") && checkState === 0)
                                            players.splice(players.indexOf("FUNIMATION"), 1)
                                        else if(checkState === 2 && !detection.includes("FUNIMATION"))
                                            players.unshift("FUNIMATION")
                                        configDiretorio.players = players
                                    }
                                }

                                CheckBox {
                                    id: checkBox9
                                    width: 165
                                    text: qsTr("MPV")
                                    Component.onCompleted:{
                                        if(fcheckState("mediaPlayers", text.replace(" ","").toUpperCase()))
                                            checked = true
                                    }
                                    onCheckStateChanged: {
                                        if(players.includes("MPV") && checkState === 0)
                                            players.splice(players.indexOf("MPV"), 1)
                                        else if(checkState === 2 && !detection.includes("MPV"))
                                            players.unshift("MPV")
                                        configDiretorio.players = players
                                    }
                                }
                            }

                            Row {
                                id: row21
                                CheckBox {
                                    id: checkBox11
                                    width: 125
                                    text: qsTr("Hulu")
                                    Component.onCompleted:{
                                        if(fcheckState("mediaPlayers", text.replace(" ","").toUpperCase()))
                                            checked = true
                                    }
                                    onCheckStateChanged: {
                                        if(players.includes("HULU") && checkState === 0)
                                            players.splice(players.indexOf("HULU"), 1)
                                        else if(checkState === 2 && !detection.includes("HULU"))
                                            players.unshift("HULU")
                                        configDiretorio.players = players
                                    }
                                }

                                CheckBox {
                                    id: checkBox12
                                    width: 165
                                    text: qsTr("VLC")
                                    Component.onCompleted:{
                                        if(fcheckState("mediaPlayers", text.replace(" ","").toUpperCase()))
                                            checked = true
                                    }
                                    onCheckStateChanged: {
                                        if(players.includes("VLC") && checkState === 0)
                                            players.splice(players.indexOf("VLC"), 1)
                                        else if(checkState === 2 && !detection.includes("VLC"))
                                            players.unshift("VLC")
                                        configDiretorio.players = players
                                    }
                                }
                            }

                            Row {
                                id: row22
                                CheckBox {
                                    id: checkBox14
                                    width: 125
                                    text: qsTr("KissAnime")
                                    Component.onCompleted:{
                                        if(fcheckState("mediaPlayers", text.replace(" ","").toUpperCase()))
                                            checked = true
                                    }
                                    onCheckStateChanged: {
                                        if(players.includes("KISSANIME") && checkState === 0)
                                            players.splice(players.indexOf("KISSANIME"), 1)
                                        else if(checkState === 2 && !detection.includes("KISSANIME"))
                                            players.unshift("KISSANIME")
                                        configDiretorio.players = players
                                    }
                                }

                                CheckBox {
                                    id: checkBox15
                                    width: 165
                                    text: qsTr("Winamp")
                                    Component.onCompleted:{
                                        if(fcheckState("mediaPlayers", text.replace(" ","").toUpperCase()))
                                            checked = true
                                    }
                                    onCheckStateChanged: {
                                        if(players.includes("WINAMP") && checkState === 0)
                                            players.splice(players.indexOf("WINAMP"), 1)
                                        else if(checkState === 2 && !detection.includes("WINAMP"))
                                            players.unshift("WINAMP")
                                        configDiretorio.players = players
                                    }
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
                                    id: qualityTrue
                                    text: qsTr("Yes")
                                    onClicked: configDiretorio.lowQuality = true
                                }

                                RadioButton {
                                    id: qualityFalse
                                    text: qsTr("No")
                                    onClicked: configDiretorio.lowQuality = false
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
    D{i:0;formeditorZoom:0.8999999761581421}D{i:8;anchors_height:160;anchors_width:110}
D{i:35;anchors_height:200;anchors_width:200}D{i:36;anchors_height:400;anchors_width:200}
D{i:31;anchors_height:400;anchors_width:200}D{i:30;anchors_height:200;anchors_width:200}
D{i:39;anchors_height:200;anchors_width:200}D{i:45;anchors_height:400;anchors_width:200}
D{i:44;anchors_height:200;anchors_width:200}D{i:40;anchors_height:400;anchors_width:200}
D{i:1;anchors_height:400;anchors_width:200}
}
##^##*/
