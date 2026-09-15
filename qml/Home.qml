import QtQuick 2.12

Item {
    id: home
    width: 1920
    height: 814
    scale: 1


    property var imagem: ""

    Row {
        id: row4
        anchors.fill: parent

        Column {
            id: column
            width: 834
            height: 814

            Row {
                id: row
                width: parent.width
                height: parent.height-208


                Column {
                    id: column2
                    width: 8
                    height: parent.height
                }

                Column {
                    id: column3
                    width: parent.width-8
                    height: parent.height

                    AnimeGrid {
                        id: animeGrid
                    }
                }
            }


            Row {
                id: row5
                width: parent.width
                height: 7
            }
            Row {
                id: row1
                y: 606
                width: 834
                height: 201
                leftPadding: 8

                SlotBaixoEsquerda {
                    id: slotBaixoEsquerda
                    x: 834
                    y: 0
                    width: 796
                    height: parent.height
                }
            }
        }

        Column {
            id: column1
            width: parent.width-column.width
            height: parent.height


            Row {
                id: row3
                width: 1086
                height: 606

                Item {
                    id: element1
                    width: 1086
                    height: 606

                    AnimeSelecionado {

                        id: animeSelecionado
                        x: 0
                        y: 0
                        idAnime: idAnimeSelecionado
                        onIdAnimeChanged: {
                            if(idAnime !== "null")
                                imagem = "file:///"+imagemAnimeSelecionado
                            else
                                imagem = ""
                        }
                        imagemSelecionada: imagem
                        anchors.fill: parent
                    }
                }
            }

            Row {
                id: row2
                y: 606
                width: parent.width
                height: 208


                Column {
                    id: column5
                    width: parent.width-column4.width
                    height: parent.height
                }
                Column {
                    id: column4
                    width: 417
                    height: parent.height

                    AnimeSupportButtons {
                        id: animeSupportButtons
                        anchors.fill: parent
                        anchors.rightMargin: 7
                    }
                }
            }

        }
    }


}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.6600000262260437}D{i:4}D{i:5}D{i:7}D{i:9}D{i:8}D{i:2}D{i:13}
D{i:12}D{i:11}D{i:10}D{i:1}
}
##^##*/
