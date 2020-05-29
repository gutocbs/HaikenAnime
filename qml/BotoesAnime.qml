import QtQuick 2.0
import QtQuick.Controls 2.3
import Qt.labs.calendar 1.0

Item {
    id: element
    width: 666
    height: 190

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
                        id: rectangle1
                        x: 0
                        y: 0
                        color: "#1bd36a"
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

                                Label {
                                    id: label2
                                    x: 0
                                    y: 0
                                    width: parent.width-5
                                    height: column12.width
                                    text: qsTr("Progresso")
                                    anchors.verticalCenter: parent.verticalCenter
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                    font.pointSize: 8
                                    font.family: "Tahoma"
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

                        Label {
                            id: label3
                            x: 0
                            y: 7
                            width: parent.width
                            height: column12.width
                            text: qsTr("xy/zw")
                            anchors.verticalCenter: parent.verticalCenter
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignLeft
                            font.pointSize: 8
                            font.family: "Tahoma"
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
                                    id: button5
                                    x: 0
                                    y: 10
                                    width: parent.width
                                    height: parent.width
                                    text: qsTr("+")
                                    font.pointSize: 8
                                    font.family: "Tahoma"
                                    anchors.verticalCenter: parent.verticalCenter
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
                                    id: button6
                                    x: 0
                                    y: 10
                                    width: parent.width
                                    height: parent.width
                                    text: qsTr("-")
                                    anchors.verticalCenter: parent.verticalCenter
                                    font.pointSize: 8
                                    font.family: "Tahoma"
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

                                Label {
                                    id: label1
                                    x: 0
                                    y: 0
                                    width: parent.width-5
                                    height: column12.width
                                    text: qsTr("Nota")
                                    anchors.verticalCenter: parent.verticalCenter
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                    font.pointSize: 8
                                    font.family: "Tahoma"
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

                        Label {
                            id: label4
                            x: 0
                            y: 7
                            width: parent.width
                            height: column12.width
                            text: qsTr("xy/zw")
                            anchors.verticalCenter: parent.verticalCenter
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignLeft
                            font.pointSize: 8
                            font.family: "Tahoma"
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
                                    id: button7
                                    x: 0
                                    y: 10
                                    width: parent.width
                                    height: parent.width
                                    text: qsTr("+")
                                    anchors.verticalCenter: parent.verticalCenter
                                    font.pointSize: 8
                                    font.family: "Tahoma"
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
                                    id: button8
                                    x: 0
                                    y: 10
                                    width: parent.width
                                    height: parent.width
                                    text: qsTr("-")
                                    anchors.verticalCenter: parent.verticalCenter
                                    font.pointSize: 8
                                    font.family: "Tahoma"
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
                            id: column13
                            x: column12.width+(parent.width/20)
                            width: parent.width-column12.width-(parent.width/20)
                            height: parent.height

                            Item {
                                id: element18
                                width: parent.width
                                height: parent.height

                                Label {
                                    id: label
                                    x: 5
                                    y: 45
                                    width: parent.width-5
                                    height: column12.width
                                    text: qsTr("Mudar para Lista")
                                    font.pointSize: 8
                                    font.family: "Tahoma"
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                    anchors.verticalCenter: parent.verticalCenter
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
    D{i:0;formeditorZoom:1.5}D{i:1;anchors_height:152;anchors_width:578}D{i:9;anchors_height:36;anchors_width:175}
D{i:8;anchors_height:36;anchors_width:175}D{i:7;anchors_height:36;anchors_width:175}
D{i:10;anchors_height:36;anchors_width:175}D{i:15;anchors_height:36;anchors_width:36}
D{i:14;anchors_height:36;anchors_width:36}D{i:13;anchors_height:36;anchors_width:175}
D{i:16;anchors_height:200;anchors_width:200}D{i:22;anchors_height:200;anchors_width:200}
D{i:20;anchors_height:36;anchors_width:36}D{i:19;anchors_height:400;anchors_width:200}
D{i:23;anchors_height:400;anchors_width:200}D{i:30;anchors_height:400;anchors_width:69.45}
D{i:33;anchors_height:400;anchors_width:200}D{i:27;anchors_height:400;anchors_width:200}
D{i:36;anchors_height:400;anchors_width:200}D{i:34;anchors_height:400;anchors_width:200}
D{i:39;anchors_height:400;anchors_width:200}D{i:42;anchors_height:400;anchors_width:200}
D{i:38;anchors_height:400;anchors_width:69.45}D{i:25;anchors_height:400;anchors_width:200}
D{i:50;anchors_height:400;anchors_width:69.45}D{i:53;anchors_height:400;anchors_width:200}
D{i:51;anchors_height:23.427777777777777;anchors_width:66.76916666666666}D{i:47;anchors_height:400;anchors_width:200}
D{i:46;anchors_height:400;anchors_width:200}D{i:56;anchors_height:400;anchors_width:200}
D{i:54;anchors_height:400;anchors_width:200}D{i:59;anchors_height:400;anchors_width:200}
D{i:64;anchors_height:23.427777777777777;anchors_width:66.76916666666666}D{i:58;anchors_height:400;anchors_width:69.45}
D{i:57;anchors_height:23.427777777777777;anchors_width:66.76916666666666}D{i:45;anchors_height:400;anchors_width:200}
D{i:70;anchors_height:23.427777777777777;anchors_width:66.76916666666666}D{i:71;anchors_height:23.427777777777777;anchors_width:66.76916666666666}
D{i:67;anchors_height:400;anchors_width:200}D{i:79;anchors_height:23.427777777777777;anchors_width:66.76916666666666}
D{i:65;anchors_height:23.427777777777777;anchors_width:66.76916666666666}D{i:2;anchors_height:400;anchors_width:200}
}
##^##*/
