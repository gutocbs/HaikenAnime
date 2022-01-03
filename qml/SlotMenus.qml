import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Controls.Styles 1.4

Item {
    width: 1920
    height: 187

    property var nomeBotaoCurrent: "Watching"
    property var nomeBotaoCompleted: "Completed"
    property var nomeBotaoPaused: "On Hold"
    property var nomeBotaoDropped: "Dropped"
    property var nomeBotaoPlanning: "Plan to Watch"

    Rectangle {
        id: rectangle
        x: 0
        y: parent.height-(parent.height/4)-(parent.height/20)
        width: parent.width
        height: parent.height/15
        color: "#5a69a2"
        border.color: "#5a69a2"
    }

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
                    id: botaoHome
                    text: qsTr("Home")
                    display: AbstractButton.TextBesideIcon
                    font.family: "Tahoma"
                    onClicked: mainClass.buttonMenuMedia()
                }

                Button {
                    id: botaoConfiguracao
                    text: qsTr("Configuration")
                    font.family: "Tahoma"
                    onClicked: mainClass.buttonMenuConfigurations()
                }

                Button {
                    id: botaoTorrent
                    text: qsTr("Torrent")
                    font.family: "Tahoma"
                    onClicked: {
                        mainClass.buttonMenuTorrent()
                        nomeBotaoCurrent = "Refresh"
                        nomeBotaoCompleted = "Download"
                        nomeBotaoPaused = "Anime Info"
                        nomeBotaoDropped = "Torrent Info"
                        nomeBotaoPlanning = "Plan to Watch"
                        botaoPlanning.visible = true
                        boxAnos.visible = false
                    }
                }

                Button {
                    id: botaoAjuda
                    text: qsTr("Help")
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
                            id: avatar
                            width: 100
                            height: 100
                            fillMode: Image.PreserveAspectFit
                            source: imagemAvatar
                        }

                        Label {
                            id: label
                            x: 110
                            width: 200
                            height: 50
                            text: nomeUsuario
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
                    id: botaoTipoAnime
                    text: qsTr("Anime")
                    font.family: "Tahoma"
                    onClicked: {
                        mainClass.selectTypeAnime()
                        nomeBotaoCurrent = "Watching"
                        nomeBotaoCompleted = "Completed"
                        nomeBotaoPaused = "On Hold"
                        nomeBotaoDropped = "Dropped"
                        nomeBotaoPlanning = "Plan to Watch"
                        botaoPlanning.visible = true
                        boxAnos.visible = false
                    }
                }

                Button {
                    id: botaoTipoManga
                    text: qsTr("Manga")
                    font.family: "Tahoma"
                    onClicked: {
                        mainClass.selectTypeManga()
                        nomeBotaoCurrent = "Reading"
                        nomeBotaoCompleted = "Completed"
                        nomeBotaoPaused = "On Hold"
                        nomeBotaoDropped = "Dropped"
                        nomeBotaoPlanning = "Plan to Read"
                        botaoPlanning.visible = true
                        boxAnos.visible = false
                    }
                }

                Button {
                    id: botaoTipoLN
                    text: qsTr("LN")
                    font.family: "Tahoma"

                    onClicked: {
                        mainClass.selectTypeNovel()
                        nomeBotaoCurrent = "Reading"
                        nomeBotaoCompleted = "Completed"
                        nomeBotaoPaused = "On Hold"
                        nomeBotaoDropped = "Dropped"
                        nomeBotaoPlanning = "Plan to Read"
                        botaoPlanning.visible = true
                        boxAnos.visible = false

                    }
                }

                Button {
                    id: botaoSeason
                    text: qsTr("Season")
                    font.family: "Tahoma"
                    onClicked: {
                        nomeBotaoCurrent = "Winter"
                        nomeBotaoCompleted = "Spring"
                        nomeBotaoPaused = "Summer"
                        nomeBotaoDropped = "Fall"
                        botaoPlanning.visible = false
                        boxAnos.visible = true
                        mainClass.fselecionaTipoSeason(boxAnos.currentText)
                    }
                }
            }

            Row {
                id: row3
                width: parent.width
                height: parent.height/3
                spacing: 2

                Button {
                    id: botaoCurrent
                    text: nomeBotaoCurrent
                    font.family: "Tahoma"
                    onClicked: mainClass.selectListCurrent()
                }

                Button {
                    id: botaoCompleted
                    text: nomeBotaoCompleted
                    font.family: "Tahoma"
                    onClicked: mainClass.selectListCompleted()
                }

                Button {
                    id: botaoPaused
                    text: nomeBotaoPaused
                    font.family: "Tahoma"
                    onClicked: mainClass.selectListPaused()
                }

                Button {
                    id: botaoDropped
                    text: nomeBotaoDropped
                    font.family: "Tahoma"
                    onClicked: mainClass.selectListDropped()
                }

                Item {
                    id: element1
                    width: 100
                    height: 40


                    ComboBox {
                        id: boxAnos
                        width: 100
                        textRole: "ano"
                        model: ListModel {
                            id: listModelAnos
                        }
                        Component.onCompleted: {
                            if(listModelAnos.count !== numeroAnosListas){
                                for(var i = 0; i < numeroAnosListas; i++){
                                    model.append({"ano": (2000+i).toString()})
                                }
                                boxAnos.currentIndex = 0
                            }
                        }
                        onActivated: mainClass.fselecionaTipoSeason(boxAnos.currentText)
                    }
                    Button {
                        id: botaoPlanning
                        x: 0
                        y: 0
                        text: nomeBotaoPlanning
                        font.family: "Tahoma"
                        onClicked: mainClass.selectListPlanning()
                    }
                }

                Button {
                    id: button13
                    text: qsTr("Refresh")
                    font.family: "Tahoma"
                    onClicked: mainClass.refreshMediaList()
                }

                Label {
                    id: labelTime
                    x: button13.x + button13.width + 10
                    y: -parent.height/3
                    text: time
                    renderType: Text.NativeRendering
                    font.bold: true
                    font.pointSize: 15
                    font.family: "Tahoma"
                }

            }
        }

        Column {
            id: column1
            width: parent.width-column.width
            height: parent.height

            Row {
                id: row5
                width: parent.width
                height: parent.height*2/3

                Column {
                    id: column3
                    width: parent.width-450
                    height: parent.height
                }

                Column {
                    id: column4
                    width: parent.width-column3.width
                    height: parent.height

                    Item {
                        id: element3
                        width: parent.width
                        height: parent.height

                        TextField {
                            id: searchBar
                            x: parent.width-width-8
                            y: parent.height/2
                            width: 400
                            text: qsTr("")
                            rightPadding: 6
                            renderType: Text.NativeRendering
                            font.pointSize: 10
                            font.family: "Tahoma"
                            placeholderText: "Search"
                            horizontalAlignment: Text.AlignLeft
                            selectByMouse: true
                        }
                    }
                }
            }

            Row {
                id: row6
                width: parent.width
                height: parent.height/3


                Column {
                    id: column5
                    y: 0
                    width: parent.width-450
                    height: parent.height
                }
                Column {
                    id: column2
                    width: parent.width-column5.width
                    height: parent.height

                    Item {
                        id: element2
                        width: parent.width
                        height: parent.height

                        Button {
                            id: botaoSearch
                            x: parent.width-width-8
                            y: 0
                            text: "Search"
                            font.family: "Tahoma"
                            onClicked: mainClass.buttonSearch(searchBar.text)
                        }
                    }
                }
            }
        }
    }


}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.33000001311302185}D{i:2;anchors_height:147;anchors_width:200}
}
##^##*/
