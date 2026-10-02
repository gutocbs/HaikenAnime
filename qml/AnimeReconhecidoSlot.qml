import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

Item {
    id: element
    width: 310
    height: 200
    visible: true

    property var titulo: nomeAnimeReconhecido
    property var episodios: episodiosAnimeReconhecido
    property var imagem: "file:///"+dirImagensMedias+idanimeReconhecido
    property var idAnime: idanimeReconhecido
    property var visivel: false
    property color corLabel: "#b9c9fa"

    onIdAnimeChanged: {
        if(idAnime === "")
            visivel = false
        else
            visivel = true
    }

    Row {
        id: row4
        anchors.fill: parent

        Column {
            id: column
            width: 110
            height: 200

            Item {
                id: element4
                width: 110
                height: 200
                visible: visivel

                Image {
                    id: image
                    width: 110
                    height: 150
                    source: imagem
                }

                MouseArea {
                    id: selecionaAnime
                    width: 110
                    height: 150
                    onPressed: mainClass.finfoAnimeSelecionado(0)
                }

            }
        }

        Column {
            id: column1
            width: 200
            spacing: 1
            Row {
                id: row1
                width: 150

                Item {
                    id: element5
                    width: 150
                    height: 67
                    visible: visivel


                    Rectangle {
                        id: labelNome
                        x: 0
                        y: 0
                        width: 150
                        height: 67
                        color: corLabel
                    }

                    Text {
                        id: element1
                        x: 0
                        y: 0
                        width: 150
                        height: 67
                        text: titulo
                        font.pixelSize: 12
                        textFormat: Text.AutoText
                        renderType: Text.NativeRendering
                        font.bold: false
                        font.family: "Tahoma"
                        fontSizeMode: Text.Fit
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Row {
                id: row
                width: 150
                height: 25

                Item {
                    id: element6
                    width: 150
                    height: 25
                    visible: visivel

                    Rectangle {
                        id: labelNome1
                        x: 0
                        y: 0
                        width: 150
                        height: 25
                        color: corLabel
                    }

                    Text {
                        id: element2
                        x: 0
                        y: 0
                        width: 150
                        height: 25
                        text: episodios
                        textFormat: Text.AutoText
                        wrapMode: Text.WordWrap
                        font.family: "Tahoma"
                        font.pixelSize: 12
                        font.bold: false
                        verticalAlignment: Text.AlignVCenter
                        renderType: Text.NativeRendering
                        fontSizeMode: Text.Fit
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            Row {
                id: row2
                width: 200
                height: 40

                Item {
                    id: element7
                    width: 200
                    height: 40
                    visible: visivel

                    Text {
                        id: element3
                        x: 0
                        y: 4
                        width: 200
                        height: 40
                        text: qsTr("If the anime wasn't recognized correctly, search for the right anime and click here to change!")
                        fontSizeMode: Text.Fit
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                    }
                }
            }

            Column {
                id: column2
                width: 200
                height: 65

                Row {
                    id: row5
                    width: 200
                    height: 15
                }

                DelayButton {
                    id: delayButton
                    text: qsTr("Change!")
                    visible: visivel
                }
            }
        }
    }

}



/*##^##
Designer {
    D{i:0;formeditorZoom:1.75}D{i:4;anchors_width:110}D{i:5;anchors_height:150;anchors_width:110}
D{i:17;anchors_x:0;anchors_y:4}D{i:1;anchors_height:200;anchors_width:310}
}
##^##*/
