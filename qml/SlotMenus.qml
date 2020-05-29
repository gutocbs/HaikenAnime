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
                    onClicked: mainClass.fbotaoHome()
                }

                Button {
                    id: botaoConfiguracao
                    text: qsTr("Configuration")
                    font.family: "Tahoma"
                    onClicked: mainClass.fbotaoConfig()
                }

                Button {
                    id: botaoTorrent
                    text: qsTr("Torrent")
                    font.family: "Tahoma"
                    onClicked: mainClass.fbotaoTorrent()
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
                    text: qsTr("Home")
                    font.family: "Tahoma"
                    onClicked: {
                        mainClass.fselecionaTipoAnime()
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
                        mainClass.fselecionaTipoManga()
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
                        mainClass.fselecionaTipoNovel()
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
                width: 200
                height: parent.height/3
                spacing: 2

                Button {
                    id: botaoCurrent
                    text: nomeBotaoCurrent
                    font.family: "Tahoma"
                    onClicked: mainClass.fselecionaListaCurrent()
                }

                Button {
                    id: botaoCompleted
                    text: nomeBotaoCompleted
                    font.family: "Tahoma"
                    onClicked: mainClass.fselecionaListaCompleted()
                }

                Button {
                    id: botaoPaused
                    text: nomeBotaoPaused
                    font.family: "Tahoma"
                    onClicked: mainClass.fselecionaListaPaused()
                }

                Button {
                    id: botaoDropped
                    text: nomeBotaoDropped
                    font.family: "Tahoma"
                    onClicked: mainClass.fselecionaListaDropped()
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
                        onClicked: mainClass.fselecionaListaPlanning()
                    }
                }

                Button {
                    id: button13
                    text: qsTr("Refresh")
                    font.family: "Tahoma"
                    onClicked: mainClass.frefresh()
                }

                Label {
                    id: labelTime
                    text: time
                    renderType: Text.NativeRendering
                    font.bold: true
                    font.pointSize: 15
                    font.family: "Tahoma"
                }

            }
        }
    }

}

/*##^##
Designer {
    D{i:0;formeditorZoom:1.5}D{i:9;anchors_height:400}D{i:1;anchors_height:147;anchors_width:200}
}
##^##*/
