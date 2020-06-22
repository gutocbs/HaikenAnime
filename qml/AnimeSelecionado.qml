import QtQuick 2.0

Item {
    id: element
    width: 1086
    height: 606

    property var idAnime: ""
    property var imagemSelecionada: ""

    Row {
        id: row7
        anchors.fill: parent
        Column {
            id: column1
            spacing: 1

            Row {
                id: row
                width: 666
                height: 40

                Item {
                    id: element20
                    width: 666
                    height: 40

                    Rectangle {
                        id: rectangle
                        color: "#c7d6ff"
                        anchors.fill: parent
                    }

                    Text {
                        id: element1
                        text: root.nomeAnimeSelecionado
                        fontSizeMode: Text.Fit
                        verticalAlignment: Text.AlignVCenter
                        font.family: "Tahoma"
                        anchors.fill: parent
                        font.pixelSize: 20
                    }
                }
            }

            Row {
                id: row1
                width: 666
                height: 40

                Item {
                    id: element21
                    width: 666
                    height: 40

                    Rectangle {
                        id: rectangle1
                        x: 0
                        y: 0
                        color: "#c7d6ff"
                        anchors.fill: parent
                    }

                    Text {
                        id: element2
                        x: 0
                        y: 0
                        text: root.nomeAlternativoAnimeSelecionado
                        fontSizeMode: Text.Fit
                        verticalAlignment: Text.AlignVCenter
                        font.family: "Tahoma"
                        anchors.fill: parent
                        font.pixelSize: 20
                    }
                }
            }

            Row {
                id: row2
                width: 666
                height: 40






                Column {
                    id: column2
                    width: 95
                    height: 40

                    Item {
                        id: element23
                        width: 95
                        height: 40


                        Rectangle {
                            id: rectangle2
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }
                        Text {
                            id: element5
                            x: 0
                            y: 0
                            text: qsTr("Status")
                            font.family: "Tahoma"
                            fontSizeMode: Text.Fit
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            anchors.fill: parent
                            font.pixelSize: 20
                        }
                    }
                }
                Column {
                    id: column3
                    width: 95
                    height: 40

                    Item {
                        id: element24
                        width: 95
                        height: 40

                        Rectangle {
                            id: rectangle7
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Text {
                            id: element6
                            x: 0
                            y: 0
                            text: qsTr("Nota")
                            font.family: "Tahoma"
                            fontSizeMode: Text.Fit
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            anchors.fill: parent
                            font.pixelSize: 20
                        }
                    }

                }
                Column {
                    id: column4
                    width: 95
                    height: 40

                    Item {
                        id: element22
                        width: 95
                        height: 40

                        Rectangle {
                            id: rectangle8
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Text {
                            id: element7
                            x: 0
                            y: 0
                            text: qsTr("Méda")
                            font.family: "Tahoma"
                            fontSizeMode: Text.Fit
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            anchors.fill: parent
                            font.pixelSize: 20
                        }
                    }
                }
                Column {
                    id: column5
                    width: 95
                    height: 40

                    Item {
                        id: element25
                        width: 95
                        height: 40

                        Rectangle {
                            id: rectangle9
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Text {
                            id: element8
                            x: 0
                            y: 0
                            text: qsTr("Season")
                            font.family: "Tahoma"
                            fontSizeMode: Text.Fit
                            anchors.fill: parent
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            font.pixelSize: 20
                        }
                    }
                }
                Column {
                    id: column6
                    width: 95
                    height: 40

                    Item {
                        id: element26
                        width: 95
                        height: 40

                        Rectangle {
                            id: rectangle10
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Text {
                            id: element9
                            x: 0
                            y: 0
                            text: qsTr("Air at")
                            fontSizeMode: Text.Fit
                            font.family: "Tahoma"
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            anchors.fill: parent
                            font.pixelSize: 20
                        }
                    }
                }
                Column {
                    id: column7
                    width: 95
                    height: 40

                    Item {
                        id: element27
                        width: 95
                        height: 40

                        Rectangle {
                            id: rectangle11
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Text {
                            id: element10
                            x: 0
                            y: 0
                            text: qsTr("Released")
                            font.family: "Tahoma"
                            fontSizeMode: Text.Fit
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            anchors.fill: parent
                            font.pixelSize: 20
                        }
                    }
                }

                Column {
                    id: column8
                    width: 95
                    height: 40

                    Item {
                        id: element28
                        width: 95
                        height: 40

                        Rectangle {
                            id: rectangle12
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Text {
                            id: element11
                            x: 0
                            y: 0
                            text: qsTr("Type")
                            fontSizeMode: Text.Fit
                            anchors.fill: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 20
                        }
                    }
                }
            }

            Row {
                id: row3
                width: 666
                height: 50

                Column {
                    id: column9
                    width: 95
                    height: 50

                    Item {
                        id: element29
                        width: 95
                        height: 50

                        Rectangle {
                            id: rectangle13
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Text {
                            id: element12
                            x: 0
                            y: 0
                            height: 50
                            text: root.statusAnimeSelecionado
                            font.bold: true
                            wrapMode: Text.WordWrap
                            anchors.fill: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            fontSizeMode: Text.Fit
                            font.family: "Tahoma"
                            font.pixelSize: 20
                        }
                    }
                }

                Column {
                    id: column10
                    width: 95
                    height: 50

                    Item {
                        id: element30
                        width: 95
                        height: 50

                        Rectangle {
                            id: rectangle14
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Text {
                            id: element13
                            x: 0
                            y: 0
                            text: root.mediaPessoalAnimeSelecionado
                            font.bold: true
                            wrapMode: Text.WordWrap
                            anchors.fill: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            fontSizeMode: Text.Fit
                            font.family: "Tahoma"
                            font.pixelSize: 20
                        }
                    }
                }

                Column {
                    id: column11
                    width: 95
                    height: 50

                    Item {
                        id: element31
                        width: 95
                        height: 50

                        Rectangle {
                            id: rectangle15
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Text {
                            id: element14
                            x: 0
                            y: 0
                            text: root.mediaGlobalAnimeSelecionado
                            font.bold: true
                            wrapMode: Text.WordWrap
                            anchors.fill: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            fontSizeMode: Text.Fit
                            font.family: "Tahoma"
                            font.pixelSize: 20
                        }
                    }
                }

                Column {
                    id: column12
                    width: 95
                    height: 50

                    Item {
                        id: element32
                        width: 95
                        height: 25

                        Rectangle {
                            id: rectangle16
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Text {
                            id: element15
                            text: seasonAnimeSelecionado
                            anchors.fill: parent
                            elide: Text.ElideNone
                            font.bold: true
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            fontSizeMode: Text.Fit
                            font.family: "Tahoma"
                            font.pixelSize: 20
                        }
                    }

                    Item {
                        id: element33
                        width: 95
                        height: 25

                        Rectangle {
                            id: rectangle17
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Text {
                            id: element19
                            text: seasonAnimeSelecionado
                            anchors.fill: parent
                            fontSizeMode: Text.Fit
                            font.bold: true
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            font.pixelSize: 20
                        }
                    }
                }

                Column {
                    id: column13
                    width: 95
                    height: 50

                    Item {
                        id: element34
                        width: 95
                        height: 50

                        Rectangle {
                            id: rectangle18
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Text {
                            id: element16
                            x: 0
                            y: 0
                            text: root.releaseAnimeSelecionado
                            font.bold: true
                            wrapMode: Text.WordWrap
                            anchors.fill: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            fontSizeMode: Text.Fit
                            font.family: "Tahoma"
                            font.pixelSize: 20
                        }
                    }
                }

                Column {
                    id: column14
                    width: 95
                    height: 50

                    Item {
                        id: element35
                        width: 95
                        height: 50

                        Rectangle {
                            id: rectangle19
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Text {
                            id: element17
                            x: 0
                            y: 0
                            text: root.episodiosAnimeSelecionado
                            font.bold: true
                            wrapMode: Text.WordWrap
                            anchors.fill: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            fontSizeMode: Text.Fit
                            font.family: "Tahoma"
                            font.pixelSize: 20
                        }
                    }
                }

                Column {
                    id: column15
                    width: 95
                    height: 50

                    Item {
                        id: element36
                        width: 95
                        height: 50

                        Rectangle {
                            id: rectangle20
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Text {
                            id: element18
                            x: 0
                            y: 0
                            text: root.tipoAnimeSelecionado
                            font.bold: true
                            wrapMode: Text.WordWrap
                            anchors.fill: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            fontSizeMode: Text.Fit
                            font.pixelSize: 20
                        }
                    }
                }
            }

            Row {
                id: row4
                width: 666
                height: 40

                Item {
                    id: element37
                    width: 666
                    height: 40

                    Rectangle {
                        id: rectangle4
                        x: 0
                        y: 0
                        color: "#c7d6ff"
                        anchors.fill: parent
                    }

                    Text {
                        id: element3
                        x: 0
                        y: 0
                        text: qsTr("Synopsis")
                        font.family: "Tahoma"
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.pixelSize: 20
                    }
                }
            }

            Row {
                id: row5
                width: 666
                height: 200

                Item {
                    id: element38
                    width: 666
                    height: 200

                    Rectangle {
                        id: rectangle5
                        x: 0
                        y: 0
                        color: "#c7d6ff"
                        anchors.fill: parent
                    }

                    Text {
                        id: element4
                        x: 0
                        y: 0
                        text: root.sinopseAnimeSelecionado
                        fontSizeMode: Text.Fit
                        wrapMode: Text.WordWrap
                        anchors.fill: parent
                        font.family: "Tahoma"
                        verticalAlignment: Text.AlignTop
                        font.pixelSize: 20
                    }
                }
            }

            Row {
                id: row6
                width: 666
                height: 190

                Item {
                    id: element39
                    width: 666
                    height: 190

                    Rectangle {
                        id: rectangle6
                        x: 0
                        y: 0
                        color: "#c7d6ff"
                        anchors.fill: parent
                    }

                    BotoesAnime {
                        id: botoesAnime
                        x: 0
                        y: 0
                    }
                }
            }

        }
        Column {
            id: column
            width: 420
            height: 606

            Item {
                id: element40
                width: 420
                height: 606

                Image {
                    id: image
                    anchors.fill: parent
                    source: imagemSelecionada
                    //            fillMode: Image.PreserveAspectFit
                }
            }
        }
    }

}





/*##^##
Designer {
    D{i:0;formeditorZoom:0.6600000262260437}D{i:56;anchors_x:0;anchors_y:0}D{i:59;anchors_x:0;anchors_y:25}
D{i:86;anchors_x:0;anchors_y:0}D{i:1;anchors_height:400;anchors_width:200}
}
##^##*/
