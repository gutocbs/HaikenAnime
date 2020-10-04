import QtQuick 2.0
import QtQuick.Controls 2.3
import Qt.labs.calendar 1.0

Item {
    id: element
    width: 666
    height: 190

    property var episodios: "1/12"
    property var nota: "0/10"
    property var episodioValido: false
    property var cor: "#000000"

    Rectangle {
        id: rectangle
        color: "#c7d6ff"
        anchors.fill: parent
    }

    property var novaLista: ""

    Row {
        id: row14
        anchors.fill: parent

        Column {
            id: column
            width: 175
            height: 190
            spacing: 2

            Row {
                id: row
                width: column.width
                height: (column.height/5)-2

                Item {
                    id: element1
                    width: 175
                    height: 36

                    Button {
                        id: botaoProximoEpisodio
                        text: qsTr("Próximo episódio")
                        anchors.fill: parent
                        onClicked: mainClass.fabreProximoEpisodio()
                    }
                }
            }

            Row {
                id: row1
                width: column.width
                height: (column.height/5)-2

                Item {
                    id: element2
                    width: parent.width
                    height: parent.height

                    Button {
                        id: button1
                        x: 0
                        y: 0
                        text: qsTr("Crunchyroll")
                        anchors.fill: parent
                        onClicked: mainClass.fabreStream()
                    }
                }
            }

            Row {
                id: row2
                width: column.width
                height: (column.height/5)-2

                Item {
                    id: element3
                    width: parent.width
                    height: parent.height

                    Button {
                        id: botaoAnilist
                        x: 0
                        y: 0
                        text: qsTr("Anilist")
                        anchors.fill: parent

                        onClicked: mainClass.fabreSite(idAnimeSelecionado)
                    }
                }
            }

            Row {
                id: row3
                width: column.width
                height: (column.height/5)-2

                Item {
                    id: element4
                    width: parent.width
                    height: parent.height

                    Button {
                        id: botaoAbrePasta
                        x: 0
                        y: 0
                        text: qsTr("Open Anime Folder")
                        anchors.fill: parent
                        onClicked: mainClass.fabrePastaAnime()
                    }
                }
            }

            Row {
                id: row4
                width: column.width
                height: (column.height/5)-2

                Item {
                    id: element5
                    width: parent.width
                    height: parent.height

                    Button {
                        id: button4
                        x: 0
                        y: 0
                        text: qsTr("Download")
                        anchors.fill: parent
                    }
                }
            }
        }

        Column {
            id: column1
            width: 36
            height: parent.height

            Row {
                id: row5
                width: 36
                height: 36

                Item {
                    id: element6
                    width: parent.width
                    height: parent.height

                    Rectangle {
                        id: checkEpisode
                        x: 0
                        y: 0
//                        "#1bd36a" // verde
//                        #d31b1b // Vermelho
                        color: cor
                        anchors.fill: parent
                    }
                }
            }
        }

        Column {
            id: column22
            width: parent.width - column.width - column1.width - column2.width
            height: parent.height
        }

        Column {
            id: column2
            width: parent.width-column.width-column1.width - (parent.width/20)
            height: parent.height

            Row {
                id: row6
                width: parent.width-5
                height: parent.height/5

                Column {
                    id: column3
                    width: parent.width/3
                    height: parent.height

                    Row {
                        id: row11
                        width: 139
                        height: 38
                        spacing: 10
                        Column {
                            id: column16
                            width: parent.width/6
                            height: parent.height

                            Item {
                                id: element7
                                width: parent.width
                                height: parent.height

                                Rectangle {
                                    id: rectangle4
                                    x: 0
                                    y: 0
                                    width: parent.width
                                    height: parent.width
                                    color: "#ffffff"
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }

                        Column {
                            id: column17
                            width: parent.width-column12.width-(parent.width/20)
                            height: parent.height

                            Item {
                                id: element8
                                width: parent.width
                                height: parent.height
                                Button {
                                    id: button
                                    visible: true
                                    text: qsTr("Progresso")
                                    anchors.fill: parent
                                    focusPolicy: Qt.NoFocus
                                    spacing: 0
                                    font.pixelSize: 11
                                    display: AbstractButton.TextOnly
                                    clip: false
                                    hoverEnabled: false
                                    enabled: true
                                    flat: true
                                    contentItem: Text {
                                        text: button.text
                                        font: button.font
                                        opacity: enabled ? 1.0 : 0.3
                                        color: "#000000"
                                        horizontalAlignment: Text.AlignHLeft
                                        verticalAlignment: Text.AlignVCenter
                                        wrapMode: Text.WordWrap
                                        elide: Text.ElideRight
                                        anchors.fill: parent
                                    }
                                    background: Rectangle {
                                        implicitWidth: 100
                                        implicitHeight: 40
                                        opacity: enabled ? 1 : 0.3
                                        color: "#c7d6ff"
                                        border.color: "#c7d6ff"
                                        border.width: 1
                                        radius: 2
                                    }
                                }
                            }
                        }
                    }
                }

                Column {
                    id: column4
                    width: parent.width/9
                    height: parent.height


                    Item {
                        id: element9
                        width: parent.width
                        height: parent.height

                        Button {
                            id: button3
                            visible: true
                            text: episodios
                            anchors.fill: parent
                            font.pixelSize: 12
                            flat: true
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button3.text
                                elide: Text.ElideRight
                                anchors.fill: parent
                                horizontalAlignment: Text.AlignHLeft
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                font: button3.font
                            }
                            clip: false
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitWidth: 100
                                implicitHeight: 40
                            }
                            spacing: 0
                            enabled: true
                            focusPolicy: Qt.NoFocus
                            display: AbstractButton.TextOnly
                            hoverEnabled: false
                        }
                    }
                }

                Column {
                    id: column5
                    width: parent.width/6
                    height: parent.height

                    Row {
                        id: row12
                        width: 69
                        height: 38

                        Column {
                            id: column18
                            width: parent.width/4
                            height: parent.height

                            Item {
                                id: element10
                                width: parent.width
                                height: parent.height

                                Button {
                                    id: botaoProgressoMenos
                                    x: 0
                                    y: 10
                                    width: parent.width
                                    height: parent.width
                                    text: qsTr("-")
                                    font.pointSize: 8
                                    font.family: "Tahoma"
                                    anchors.verticalCenter: parent.verticalCenter
                                    onClicked: mainClass.fmudaProgresso("diminui")
                                }
                            }
                        }

                        Column {
                            id: column19
                            width: parent.width/4
                            height: parent.height

                            Item {
                                id: element11
                                width: parent.width
                                height: parent.height

                                Button {
                                    id: botaoProgressoMais
                                    x: 0
                                    y: 10
                                    width: parent.width
                                    height: parent.width
                                    text: qsTr("+")
                                    anchors.verticalCenter: parent.verticalCenter
                                    font.pointSize: 8
                                    font.family: "Tahoma"
                                    onClicked: mainClass.fmudaProgresso("aumenta")
                                }
                            }
                        }
                    }
                }
            }

            Row {
                id: row7
                width: parent.width-5
                height: parent.height/5

                Column {
                    id: column6
                    width: parent.width/3
                    height: parent.height

                    Row {
                        id: row10
                        width: 139
                        height: 38
                        spacing: 10

                        Column {
                            id: column14
                            width: parent.width/6
                            height: parent.height

                            Item {
                                id: element12
                                width: parent.width
                                height: parent.height

                                Rectangle {
                                    id: rectangle3
                                    x: 0
                                    y: 7
                                    width: parent.width
                                    height: parent.width
                                    color: "#ffffff"
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }

                        Column {
                            id: column15
                            x: column14.width+(parent.width/20)
                            width: parent.width-column12.width-(parent.width/20)
                            height: parent.height

                            Item {
                                id: element13
                                width: parent.width
                                height: parent.height

                                Button {
                                    id: button2
                                    visible: true
                                    text: qsTr("Nota")
                                    anchors.fill: parent
                                    font.pixelSize: 11
                                    flat: true
                                    contentItem: Text {
                                        opacity: enabled ? 1.0 : 0.3
                                        color: "#000000"
                                        text: button2.text
                                        elide: Text.ElideRight
                                        anchors.fill: parent
                                        horizontalAlignment: Text.AlignHLeft
                                        verticalAlignment: Text.AlignVCenter
                                        wrapMode: Text.WordWrap
                                        font: button2.font
                                    }
                                    clip: false
                                    background: Rectangle {
                                        opacity: enabled ? 1 : 0.3
                                        color: "#c7d6ff"
                                        radius: 2
                                        border.color: "#c7d6ff"
                                        border.width: 1
                                        implicitWidth: 100
                                        implicitHeight: 40
                                    }
                                    spacing: 0
                                    enabled: true
                                    focusPolicy: Qt.NoFocus
                                    display: AbstractButton.TextOnly
                                    hoverEnabled: false
                                }
                            }
                        }
                    }
                }

                Column {
                    id: column7
                    width: parent.width/9
                    height: parent.height

                    Item {
                        id: element14
                        width: parent.width
                        height: parent.height

                        Button {
                            id: button5
                            visible: true
                            text: nota
                            anchors.fill: parent
                            font.pixelSize: 12
                            flat: true
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button5.text
                                elide: Text.ElideRight
                                anchors.fill: parent
                                horizontalAlignment: Text.AlignHLeft
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                font: button5.font
                            }
                            clip: false
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitWidth: 100
                                implicitHeight: 40
                            }
                            spacing: 0
                            enabled: true
                            focusPolicy: Qt.NoFocus
                            display: AbstractButton.TextOnly
                            hoverEnabled: false
                        }
                    }
                }

                Column {
                    id: column8
                    width: parent.width/6
                    height: parent.height

                    Row {
                        id: row13
                        width: 69.45
                        height: 38
                        Column {
                            id: column20
                            width: parent.width/4
                            height: parent.height

                            Item {
                                id: element15
                                width: parent.width
                                height: parent.height

                                Button {
                                    id: botaoNotaMenos
                                    x: 0
                                    y: 10
                                    width: parent.width
                                    height: parent.width
                                    text: qsTr("-")
                                    anchors.verticalCenter: parent.verticalCenter
                                    font.pointSize: 8
                                    font.family: "Tahoma"
                                    onClicked: mainClass.fmudaNota("diminui")
                                }
                            }
                        }

                        Column {
                            id: column21
                            width: parent.width/4
                            height: parent.height

                            Item {
                                id: element16
                                width: parent.width
                                height: parent.height

                                Button {
                                    id: botaoNotaMais
                                    x: 0
                                    y: 10
                                    width: parent.width
                                    height: parent.width
                                    text: qsTr("+")
                                    anchors.verticalCenter: parent.verticalCenter
                                    font.pointSize: 8
                                    font.family: "Tahoma"
                                    onClicked: mainClass.fmudaNota("aumenta")
                                }
                            }
                        }
                    }
                }
            }

            Row {
                id: row8
                width: parent.width-5
                height: (parent.height/5)*3

                Column {
                    id: column9
                    width: parent.width/3
                    height: parent.height

                    Row {
                        id: row9
                        width: 139
                        height: 114
                        spacing: 1

                        Column {
                            id: column12
                            width: parent.width/6
                            height: parent.height

                            Item {
                                id: element17
                                width: parent.width
                                height: parent.height

                                Rectangle {
                                    id: rectangle2
                                    x: 0
                                    y: 45
                                    width: parent.width
                                    height: parent.width
                                    color: "#ffffff"
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }

                        Column {
                            id: column23
                            width: 7
                            height: parent.height
                        }

                        Column {
                            id: column13
                            x: column12.width+(parent.width/20)
                            width: parent.width-column12.width-(parent.width/20)
                            height: parent.height

                            Item {
                                id: element18
                                width: parent.width
                                height: parent.height

//                                Label {
//                                    id: label
//                                    x: 5
//                                    y: 45
//                                    width: parent.width-5
//                                    height: column12.width
//                                    text: qsTr("Mudar para Lista")
//                                    font.pointSize: 8
//                                    font.family: "Tahoma"
//                                    verticalAlignment: Text.AlignVCenter
//                                    renderType: Text.QtRendering
//                                    horizontalAlignment: Text.AlignLeft
//                                    anchors.verticalCenter: parent.verticalCenter
//                                }

                                Button {
                                    id: button6
                                    visible: true
                                    text: qsTr("Mudar para Lista")
                                    anchors.fill: parent
                                    font.pixelSize: 11
                                    flat: true
                                    contentItem: Text {
                                        opacity: enabled ? 1.0 : 0.3
                                        color: "#000000"
                                        text: button6.text
                                        elide: Text.ElideRight
                                        anchors.fill: parent
                                        horizontalAlignment: Text.AlignHLeft
                                        verticalAlignment: Text.AlignVCenter
                                        wrapMode: Text.WordWrap
                                        font: button6.font
                                    }
                                    clip: false
                                    background: Rectangle {
                                        opacity: enabled ? 1 : 0.3
                                        color: "#c7d6ff"
                                        radius: 2
                                        border.color: "#c7d6ff"
                                        border.width: 1
                                        implicitWidth: 100
                                        implicitHeight: 40
                                    }
                                    enabled: true
                                    spacing: 0
                                    hoverEnabled: false
                                    display: AbstractButton.TextOnly
                                    focusPolicy: Qt.NoFocus
                                }
                            }
                        }

                    }
                }

                Column {
                    id: column10
                    width: parent.width/3
                    height: parent.height

                    Item {
                        id: element19
                        width: parent.width
                        height: parent.height

                        ComboBox {
                            id: comboBoxMudaLista
                            x: 0
                            y: 45
                            height: column12.width
                            font.family: "Tahoma"
                            anchors.verticalCenter: parent.verticalCenter
                            model: ['Watching', 'Completed', 'On Hold', 'Dropped', 'Plan to Watch']
                        }
                    }
                }

                Column {
                    id: column11
                    width: parent.width/6
                    height: parent.height

                    Item {
                        id: element20
                        width: parent.width
                        height: parent.height

                        DelayButton {
                            id: botaoConfirmarLista
                            x: 3
                            y: 45
                            width: parent.width-parent.width/20
                            height: column12.width
                            text: qsTr("Confirm")
                            anchors.verticalCenter: parent.verticalCenter
                            onActivated: {
                                if(comboBoxMudaLista.currentText === 'Watching')
                                    novaLista = 'CURRENT'
                                else if(comboBoxMudaLista.currentText === 'Completed')
                                    novaLista = 'COMPLETED'
                                else if(comboBoxMudaLista.currentText === 'On Hold')
                                    novaLista = 'PAUSED'
                                else if(comboBoxMudaLista.currentText === 'Dropped')
                                    novaLista = 'DROPPED'
                                else if(comboBoxMudaLista.currentText === 'Plan to Watch')
                                    novaLista = 'PLANNING'
                                mainClass.fmudaListaAnime(novaLista)
                                progress = 0
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
    D{i:0;formeditorZoom:1.5}D{i:1}D{i:9}D{i:8}D{i:7}D{i:10}D{i:15}D{i:14}D{i:13}D{i:16}
D{i:22}D{i:20}D{i:19}D{i:23}D{i:30}D{i:27}D{i:36}D{i:43}D{i:46}D{i:42}D{i:25}D{i:54}
D{i:55}D{i:51}D{i:50}D{i:60}D{i:67}D{i:72}D{i:66}D{i:65}D{i:49}D{i:78}D{i:80}D{i:75}
D{i:90}D{i:73}D{i:2}
}
##^##*/
