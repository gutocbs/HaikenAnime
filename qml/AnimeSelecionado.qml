import QtQuick 2.0
import QtQuick.Controls 2.3

Item {
    id: element
    width: 1086
    height: 606

    property string idAnime: ""
    property string imagemSelecionada: ""

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

                    Button {
                        id: button
                        visible: true
                        text: root.nomeAnimeSelecionado
                        anchors.fill: parent
                        focusPolicy: Qt.NoFocus
                        spacing: 0
                        font.pixelSize: 20
                        font.bold: true
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

            Row {
                id: row1
                width: 666
                height: 40

                Item {
                    id: element21
                    width: 666
                    height: 40

                    Button {
                        id: button1
                        visible: true
                        text: root.nomeAlternativoAnimeSelecionado
                        anchors.fill: parent
                        font.pixelSize: 20
                        focusPolicy: Qt.NoFocus
                        contentItem: Text {
                            opacity: enabled ? 1.0 : 0.3
                            color: "#000000"
                            text: button1.text
                            elide: Text.ElideRight
                            fontSizeMode: Text.Fit
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            font: button1.font
                        }
                        flat: true
                        clip: false
                        display: AbstractButton.TextOnly
                        spacing: 0
                        enabled: true
                        background: Rectangle {
                            opacity: enabled ? 1 : 0.3
                            color: "#c7d6ff"
                            radius: 2
                            border.color: "#c7d6ff"
                            border.width: 1
                            implicitHeight: 40
                            implicitWidth: 100
                        }
                        hoverEnabled: false
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
                            id: rectangle7
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }
                        Button {
                            id: button2
                            visible: true
                            text: qsTr("Status")
                            anchors.fill: parent
                            font.pixelSize: 20
                            focusPolicy: Qt.NoFocus
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button2.text
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                fontSizeMode: Text.Fit
                                font: button2.font
                            }
                            flat: true
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
                            hoverEnabled: false
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
                            id: rectangle8
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }
                        Button {
                            id: button3
                            visible: true
                            text: qsTr("Nota")
                            anchors.fill: parent
                            font.pixelSize: 20
                            focusPolicy: Qt.NoFocus
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button3.text
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                fontSizeMode: Text.Fit
                                font: button3.font
                            }
                            flat: true
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
                            hoverEnabled: false
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
                            id: rectangle9
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }
                        Button {
                            id: button4
                            visible: true
                            text: qsTr("Media")
                            anchors.fill: parent
                            font.pixelSize: 20
                            focusPolicy: Qt.NoFocus
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button4.text
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                fontSizeMode: Text.Fit
                                font: button4.font
                            }
                            flat: true
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
                            hoverEnabled: false
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
                            id: rectangle10
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }
                        Button {
                            id: button5
                            visible: true
                            text: qsTr("Season")
                            anchors.fill: parent
                            font.pixelSize: 20
                            focusPolicy: Qt.NoFocus
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button5.text
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                fontSizeMode: Text.Fit
                                font: button5.font
                            }
                            flat: true
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
                            hoverEnabled: false
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
                            id: rectangle11
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }
                        Button {
                            id: button6
                            visible: true
                            text: qsTr("Air at")
                            anchors.fill: parent
                            font.pixelSize: 20
                            focusPolicy: Qt.NoFocus
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button6.text
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                fontSizeMode: Text.Fit
                                font: button6.font
                            }
                            flat: true
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
                            hoverEnabled: false
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
                            id: rectangle12
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Button {
                            id: button7
                            x: 0
                            y: 0
                            visible: true
                            text: qsTr("Released")
                            anchors.fill: parent
                            font.pixelSize: 20
                            focusPolicy: Qt.NoFocus
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button7.text
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                fontSizeMode: Text.Fit
                                font: button7.font
                            }
                            flat: true
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
                            hoverEnabled: false
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
                            id: rectangle13
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }
                        Button {
                            id: button8
                            visible: true
                            text: qsTr("Type")
                            anchors.fill: parent
                            font.pixelSize: 20
                            focusPolicy: Qt.NoFocus
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button8.text
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                fontSizeMode: Text.Fit
                                font: button8.font
                            }
                            flat: true
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
                            hoverEnabled: false
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
                            id: rectangle14
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }
                        Button {
                            id: button9
                            visible: true
                            text: root.statusAnimeSelecionado
                            anchors.fill: parent
                            font.pixelSize: 15
                            focusPolicy: Qt.NoFocus
                            font.bold: true
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                visible: true
                                color: "#000000"
                                text: button9.text
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                minimumPointSize: 11
                                minimumPixelSize: 11
                                elide: Text.ElideRight
                                anchors.fill: parent
                                fontSizeMode: Text.Fit
                                font.family: "Tahoma"
                                font.bold: true
                                font.pointSize: 11
                            }
                            flat: true
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
                            hoverEnabled: false
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
                            id: rectangle15
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }
                        Button {
                            id: button10
                            visible: true
                            text: root.mediaPessoalAnimeSelecionado
                            anchors.fill: parent
                            font.pixelSize: 20
                            focusPolicy: Qt.NoFocus
                            font.bold: true
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button10.text
                                elide: Text.ElideRight
                                anchors.fill: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                font.bold: true
                                font.pointSize: 15
                                font.family: "Tahoma"
                                minimumPointSize: 11
                                minimumPixelSize: 11
                                fontSizeMode: Text.Fit
                                //                                font: button10.font
                            }
                            flat: true
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
                            hoverEnabled: false
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
                            id: rectangle16
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }
                        Button {
                            id: button19
                            visible: true
                            text: root.mediaGlobalAnimeSelecionado
                            anchors.fill: parent
                            font.pixelSize: 20
                            hoverEnabled: false
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button19.text
                                elide: Text.ElideRight
                                anchors.fill: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                font.pointSize: 15
                                font.family: "Tahoma"
                                fontSizeMode: Text.Fit
                                minimumPointSize: 11
                                minimumPixelSize: 11
                                font.bold: true
                            }
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            focusPolicy: Qt.NoFocus
                            font.bold: true
                            flat: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
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
                            id: rectangle17
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }
                        Button {
                            id: button12
                            visible: true
                            text: root.seasonAnimeSelecionado.split(" ")[0]
                            anchors.fill: parent
                            font.pixelSize: 20
                            hoverEnabled: false
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button12.text
                                elide: Text.ElideRight
                                anchors.fill: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                font.pointSize: 11
                                font.family: "Tahoma"
                                fontSizeMode: Text.Fit
                                minimumPointSize: 11
                                minimumPixelSize: 11
                                font.bold: true
                            }
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            focusPolicy: Qt.NoFocus
                            font.bold: true
                            flat: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
                        }
                    }

                    Item {
                        id: element33
                        width: 95
                        height: 25


                        Rectangle {
                            id: rectangle18
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }
                        Button {
                            id: button20
                            visible: true
                            text: seasonAnimeSelecionado.split(" ")[1]
                            anchors.fill: parent
                            font.pixelSize: 20
                            hoverEnabled: false
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button20.text
                                elide: Text.ElideRight
                                anchors.fill: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                font.pointSize: 11
                                font.family: "Tahoma"
                                fontSizeMode: Text.Fit
                                minimumPointSize: 11
                                minimumPixelSize: 11
                                font.bold: true
                            }
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            focusPolicy: Qt.NoFocus
                            font.bold: true
                            flat: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
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
                            id: rectangle19
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }
                        Button {
                            id: button21
                            visible: true
                            text: root.releaseAnimeSelecionado
                            anchors.fill: parent
                            font.pixelSize: 20
                            hoverEnabled: false
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button21.text
                                elide: Text.ElideRight
                                anchors.fill: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                font.pointSize: 11
                                font.family: "Tahoma"
                                fontSizeMode: Text.Fit
                                minimumPointSize: 11
                                minimumPixelSize: 11
                                font.bold: true
                            }
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            focusPolicy: Qt.NoFocus
                            font.bold: true
                            flat: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
                        }
                    }
                }

                Column {
                    id: column14
                    width: 95
                    height: 50

                    Item {
                        id: element41
                        width: 95
                        height: 25
                        Rectangle {
                            id: rectangle22
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Button {
                            id: button13
                            visible: true
                            text: root.episodiosAnimeSelecionado.split(" ")[0]
                            anchors.fill: parent
                            font.pixelSize: 20
                            hoverEnabled: false
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button13.text
                                elide: Text.ElideRight
                                anchors.fill: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                font.pointSize: 11
                                font.family: "Tahoma"
                                fontSizeMode: Text.Fit
                                minimumPointSize: 11
                                minimumPixelSize: 11
                                font.bold: true
                            }
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            focusPolicy: Qt.NoFocus
                            font.bold: true
                            flat: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
                        }
                    }

                    Item {
                        id: element42
                        width: 95
                        height: 25
                        Rectangle {
                            id: rectangle23
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }

                        Button {
                            id: button24
                            visible: true
                            text: root.episodiosAnimeSelecionado.split(" ")[1]
                            anchors.fill: parent
                            font.pixelSize: 20
                            hoverEnabled: false
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button24.text
                                elide: Text.ElideRight
                                anchors.fill: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                font.pointSize: 11
                                font.family: "Tahoma"
                                fontSizeMode: Text.Fit
                                minimumPointSize: 11
                                minimumPixelSize: 11
                                font.bold: true
                            }
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            focusPolicy: Qt.NoFocus
                            font.bold: true
                            flat: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
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
                            id: rectangle21
                            x: 0
                            y: 0
                            color: "#c7d6ff"
                            anchors.fill: parent
                        }
                        Button {
                            id: button23
                            visible: true
                            text: root.tipoAnimeSelecionado
                            anchors.fill: parent
                            font.pixelSize: 20
                            hoverEnabled: false
                            contentItem: Text {
                                opacity: enabled ? 1.0 : 0.3
                                color: "#000000"
                                text: button23.text
                                elide: Text.ElideRight
                                anchors.fill: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                font.pointSize: 11
                                font.family: "Tahoma"
                                fontSizeMode: Text.Fit
                                minimumPointSize: 11
                                minimumPixelSize: 11
                                font.bold: true
                            }
                            clip: false
                            display: AbstractButton.TextOnly
                            spacing: 0
                            enabled: true
                            focusPolicy: Qt.NoFocus
                            font.bold: true
                            flat: true
                            background: Rectangle {
                                opacity: enabled ? 1 : 0.3
                                color: "#c7d6ff"
                                radius: 2
                                border.color: "#c7d6ff"
                                border.width: 1
                                implicitHeight: 40
                                implicitWidth: 100
                            }
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

                    Button {
                        id: button17
                        visible: true
                        text: qsTr("Synopsis")
                        anchors.fill: parent
                        font.pixelSize: 20
                        focusPolicy: Qt.NoFocus
                        contentItem: Text {
                            opacity: enabled ? 1.0 : 0.3
                            color: "#000000"
                            text: button17.text
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignHLeft
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            font: button17.font
                        }
                        flat: true
                        clip: false
                        display: AbstractButton.TextOnly
                        spacing: 0
                        enabled: true
                        background: Rectangle {
                            opacity: enabled ? 1 : 0.3
                            color: "#c7d6ff"
                            radius: 2
                            border.color: "#c7d6ff"
                            border.width: 1
                            implicitHeight: 40
                            implicitWidth: 100
                        }
                        hoverEnabled: false
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

                    Button {
                        id: button18
                        visible: true
                        text: root.sinopseAnimeSelecionado
                        anchors.fill: parent
                        font.pixelSize: 20
                        focusPolicy: Qt.NoFocus
                        contentItem: Text {
                            opacity: enabled ? 1.0 : 0.3
                            color: "#000000"
                            text: button18.text
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignHLeft
                            verticalAlignment: Text.AlignTop
                            wrapMode: Text.WordWrap
                            font: button18.font
                        }
                        flat: true
                        clip: false
                        display: AbstractButton.TextOnly
                        spacing: 0
                        enabled: true
                        background: Rectangle {
                            opacity: enabled ? 1 : 0.3
                            color: "#c7d6ff"
                            radius: 2
                            border.color: "#c7d6ff"
                            border.width: 1
                            implicitHeight: 40
                            implicitWidth: 100
                        }
                        hoverEnabled: false
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
                        episodios: root.episodiosAssistidosAnimeSelecionado + "/" + root.episodiosTotaisAnimeSelecionado
                        nota: root.mediaPessoalAnimeSelecionado + "/10"
                        cor: root.proximoEpisodioAnimeSelecionado
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
    D{i:0;formeditorZoom:0.6600000262260437}
}
##^##*/
