import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

Item {
    id: slotAnime
    width: 260
    height: 150
    visible: true

    property var titulo: "What is Lorem Ipsum?"
    property var episodios: "1/12"
    property var nota: "7/10"
    property var posicao: 0
    property var lista: "Watching"
    property var imagem: "qrc:/qtquickplugin/images/template_image.png"
    property color corLabel: "#b9c9fa"
    property color corEpisodio: "black"
    property var idAnimeSlot: ""

    Row {
        id: row
        anchors.fill: parent

        Column {
            id: column
            width: 110
            height: 150

            Item {
                id: element5
                width: 110
                height: 150

                Image {
                    id: image
                    x: 0
                    y: 0
                    width: 110
                    height: 150
                    source: imagem
                    //            fillMode: Image.PreserveAspectFit
                }

                MouseArea {
                    id: selecionaAnime
                    width: 110
                    height: 150
                    onPressed: mainClass.finfoAnimeSelecionado(posicao)
                }
            }


            //        Rectangle {
            //            id: rectangle4
            //            color: "#df4f4f"
            //            anchors.fill: parent
            //        }
        }

        Column {
            id: column1
            spacing: 1
            Row {
                id: row1
                width: 150
                height: 50

                Item {
                    id: element6
                    width: 150
                    height: 50

                    Rectangle {
                        id: labelNome
                        x: 0
                        y: 0
                        width: 150
                        height: 50
                        color: corLabel
                    }

                    Text {
                        id: element1
                        x: 0
                        y: 0
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
                        minimumPixelSize: 20
                        anchors.fill: parent
                    }
                }
            }



            Row {
                id: row2
                width: 150
                height: 25

                Item {
                    id: element7
                    width: 150
                    height: 25

                    Rectangle {
                        id: rectangle1
                        x: 0
                        y: 0
                        color: corLabel
                        anchors.fill: parent
                    }

                    Text {
                        id: element2
                        x: 0
                        y: 0
                        text: episodios
                        renderType: Text.NativeRendering
                        font.family: "tahoma"
                        fontSizeMode: Text.Fit
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.NoWrap
                        minimumPixelSize: 20
                        horizontalAlignment: Text.AlignHCenter
                        anchors.fill: parent
                        font.pixelSize: 12
                        color: corEpisodio
                    }
                }
            }

            Row {
                id: row3
                width: 150
                height: 25


                Item {
                    id: element8
                    width: 150
                    height: 25

                    Rectangle {
                        id: rectangle2
                        x: 0
                        y: 0
                        color: corLabel
                        anchors.fill: parent
                    }

                    Text {
                        id: element3
                        x: 0
                        y: 0
                        text: nota
                        renderType: Text.NativeRendering
                        anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        minimumPixelSize: 20
                        fontSizeMode: Text.Fit
                        horizontalAlignment: Text.AlignHCenter
                        font.family: "tahoma"
                        font.pixelSize: 12
                    }
                }
            }

            Row {
                id: row4
                width: 150
                height: 25

                Item {
                    id: element9
                    width: 150
                    height: 25

                    Rectangle {
                        id: rectangle3
                        x: 0
                        y: -103
                        color: corLabel
                        anchors.fill: parent
                    }

                    Text {
                        id: element4
                        x: 0
                        y: -103
                        text: lista
                        renderType: Text.NativeRendering
                        anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        minimumPixelSize: 20
                        fontSizeMode: Text.Fit
                        horizontalAlignment: Text.AlignHCenter
                        font.family: "tahoma"
                        font.pixelSize: 12
                    }
                }
            }

        }

    }

}



/*##^##
Designer {
    D{i:0;formeditorZoom:1.75}D{i:1}
}
##^##*/
