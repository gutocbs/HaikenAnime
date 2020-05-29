import QtQuick 2.0
import QtQuick.Controls 2.3

Item {
    width: 460
    height: 814

    property bool authorizationBool: false

    Column {
        id: column
        anchors.fill: parent

        Row {
            id: row
            width: parent.width
            height: 50

            Item {
                id: element
                width: parent.width
                height: parent.height

                Label {
                    id: label
                    text: qsTr("User configuration")
                    font.pointSize: 14
                    font.bold: true
                    font.family: "Tahoma"
                }
            }
        }

        Row {
            id: row1
            width: parent.width
            height: 50

            Column {
                id: column1
                width: 90
                height: parent.height

                Item {
                    id: element1
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label1
                        text: qsTr("Service")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.pointSize: 14
                        font.family: "Tahoma"
                        renderType: Text.NativeRendering
                    }
                }
            }

            Column {
                id: column2
                width: 200
                height: parent.height
                Item {
                    id: element3
                    width: parent.width
                    height: parent.height

                    ComboBox {
                        id: service
                        model: ["Anilist", "MyAnimeList", "Kitsu"]
                        onActivated: mainClass.fsetClient(service.currentText)
                    }
                }
            }
        }

        Row {
            id: row2
            width: parent.width
            height: 50

            Column {
                id: column3
                width: 90
                height: parent.height

                Item {
                    id: element2
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label2
                        text: qsTr("User")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.pointSize: 14
                        font.family: "Tahoma"
                    }
                }
            }

            Column {
                id: column4
                width: 200
                height: parent.height

                Item {
                    id: element4
                    width: parent.width
                    height: parent.height

                    TextField {
                        id: usernameText
                        width: 210
                        text: nomeUsuario
                    }
                }
            }
        }

        Row {
            id: row3
            width: parent.width
            height: 50

            Column {
                id: column5
                width: 200
                height: parent.height

                Item {
                    id: element5
                    width: parent.width
                    height: parent.height

                    Button {
                        id: button
                        x: parent.width- width -10
                        text: qsTr("Make Account")
                        leftPadding: 8
                        onClicked: Qt.openUrlExternally("https://anilist.co/signup");
                    }
                }
            }

            Column {
                id: column6
                width: 200
                height: parent.height
                Item {
                    id: element6
                    height: parent.height
                    Button {
                        id: button1
                        text: qsTr("Authorize")
                        onClicked: authorizationBool = true
                    }
                }
            }
        }

        Row {
            id: row4
            width: parent.width
            height: 50
            Column {
                id: column7
                width: 200
                height: parent.height
                Item {
                    id: element7
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label3
                        text: qsTr("Authorization code")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.pointSize: 14
                        visible: authorizationBool
                    }
                }
            }
        }

        Row {
            id: row5
            width: parent.width
            height: 100

            Column {
                id: column10
                width: parent.width-20
                height: parent.height
                Item {
                    id: element10
                    width: parent.width
                    height: parent.height


                    Rectangle {
                        id: rectangle
                        color: "#ffffff"
                        anchors.fill: parent
                        visible: authorizationBool
                    }
                    TextArea {
                        id: authCodeArea
                        text: qsTr("Authorization Code")
                        renderType: Text.NativeRendering
                        anchors.fill: parent
                        font.family: "Tahoma"
                        placeholderText: "Authorization Code"
                        wrapMode: Text.WordWrap
                        visible: authorizationBool
                    }
                }
            }
        }

        Row {
            id: row6
            width: parent.width
            height: 50
            spacing: 4

            Column {
                id: column12
                width: parent.width
                height: parent.height
                Item {
                    id: element12
                    height: parent.height
                    Button {
                        id: button3
                        x: 340
                        y: 4
                        text: qsTr("Save Code")
                        visible: authorizationBool
                        onClicked: mainClass.fsetauthCode(authCodeArea.text)
                    }
                }
            }
        }

        Row {
            id: row7
            width: parent.width
            height: 260
            Column {
                id: column13
                width: parent.width
                height: parent.height

                Row {
                    id: row8
                    width: parent.width
                    height: 50

                    Column {
                        id: column9
                        width: 120
                        height: parent.height

                        Item {
                            id: element8
                            width: parent.width
                            height: parent.height

                            Button {
                                id: button2
                                text: "History"
                                onClicked: stackViewConfig.push("ConfHistorico.qml")
                            }
                        }
                    }

                    Column {
                        id: column11
                        width: 200
                        height: parent.height
                        Item {
                            id: element9
                            width: parent.width
                            height: parent.height

                            Label {
                                id: label4
                                text: qsTr("Label")
                            }
                        }
                    }
                }

                Row {
                    id: row11
                    width: parent.width
                    height: 50
                    Column {
                        id: column16
                        width: 120
                        height: parent.height
                        Item {
                            id: element11
                            width: parent.width
                            height: parent.height
                            Button {
                                id: botaoConfigTorrent
                                text: qsTr("Torrent")
                                onClicked: stackViewConfig.push("ConfigTorrentSlot.qml")
                            }
                        }
                    }

                    Column {
                        id: column17
                        width: 200
                        height: parent.height
                        Item {
                            id: element15
                            width: parent.width
                            height: parent.height
                            Label {
                                id: label5
                                text: qsTr("Label")
                            }
                        }
                    }
                }

                Row {
                    id: row12
                    width: parent.width
                    height: 50
                    Column {
                        id: column18
                        width: 120
                        height: parent.height
                        Item {
                            id: element16
                            width: parent.width
                            height: parent.height
                            Button {
                                id: button7
                                text: qsTr("Filters")
                                onClicked: stackViewConfig.push("ConfFiltros.qml")
                            }
                        }
                    }

                    Column {
                        id: column19
                        width: 200
                        height: parent.height
                        Item {
                            id: element17
                            width: parent.width
                            height: parent.height
                            Label {
                                id: label6
                                text: qsTr("Label")
                            }
                        }
                    }
                }

                Row {
                    id: row13
                    width: parent.width
                    height: 50
                    Column {
                        id: column20
                        width: 120
                        height: parent.height
                        Item {
                            id: element18
                            width: parent.width
                            height: parent.height
                            Button {
                                id: button8
                                text: qsTr("Recognition")
                                onClicked: stackViewConfig.push("ConfDiretorio.qml")
                            }
                        }
                    }

                    Column {
                        id: column21
                        width: 200
                        height: parent.height
                        Item {
                            id: element19
                            width: parent.width
                            height: parent.height
                            Label {
                                id: label7
                                text: qsTr("Label")
                            }
                        }
                    }
                }

                Row {
                    id: row14
                    width: parent.width
                    height: 50
                    Column {
                        id: column22
                        width: 120
                        height: parent.height
                        Item {
                            id: element20
                            width: parent.width
                            height: parent.height
                            Button {
                                id: button9
                                text: qsTr("Color and Font")
                                onClicked: stackViewConfig.push("ConfigFont.qml")
                            }
                        }
                    }

                    Column {
                        id: column23
                        width: 200
                        height: parent.height
                        Item {
                            id: element21
                            width: parent.width
                            height: parent.height
                            Label {
                                id: label8
                                text: qsTr("Label")
                            }
                        }
                    }
                }
            }
            spacing: 4
        }

        Row {
            id: row9
            width: parent.width
            height: 50
            Column {
                id: column14
                width: 200
                height: parent.height
                Item {
                    id: element13
                    height: parent.height
                    Button {
                        id: button4
                        text: qsTr("Save Code")
                    }
                }
            }
            spacing: 4
        }

        Row {
            id: row10
            width: parent.width
            height: 50
            Column {
                id: column15
                width: 200
                height: parent.height
                Item {
                    id: element14
                    height: parent.height
                    Button {
                        id: button5
                        text: qsTr("Save")
                        onClicked: {
                            mainClass.fsetUsername(usernameText.text)
                        }
                    }
                }
            }
            spacing: 4
        }
    }

}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.8999999761581421}D{i:33;anchors_height:200;anchors_width:200}
D{i:1;anchors_height:400;anchors_width:200}
}
##^##*/
