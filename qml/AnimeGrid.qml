import QtQuick 2.0

Item {
    id: element
    width: 804
    height: 606
    property alias row1Y: row1.y
    property color corOriginal: "#b9c9fa"
    property color corTextoNormal: "black"
    property color corEpisodioEncontrado: "#357838"
    property color corEpisodioNaoEncontrado: "#783c35"
    property var localNome: ["0","1","2","3","4","5","6","7","8","9","10","11"]
    property var localEpisodios: ["0","1","2","3","4","5","6","7","8","9","10","11"]
    property var localCorEpisodios: ["0","0","0","0","0","0","0","0","0","0","0","0"]
    property var localNota: ["0","1","2","3","4","5","6","7","8","9","10","11"]
    property var localLista: ["0","1","2","3","4","5","6","7","8","9","10","11"]
    property var localCor: ["0","0","0","0","0","0","0","0","0","0","0","0"]
    property var localImagem: ["","","","","","","","","","","",""]

    function getDataFromMain(id,pos){
        var listaNomeTemp = localNome
        var listaEpisodiosTemp = localEpisodios
        var listaCorEpisodiosTemp = localCorEpisodios
        var listaNotaTemp = localNota
        var listaListaTemp = localLista
        var listaCorTemp = localCor
        var listaImaemTemp = localImagem
        if(id !== "null"){
            listaNomeTemp[pos] = nomeAnime[pos]
            listaEpisodiosTemp[pos] = episodiosAnime[pos]
            listaNotaTemp[pos] = notaAnime[pos]
            listaListaTemp[pos] = listaAnime[pos]
            listaImaemTemp[pos] = "file:///"+dirImagensMedias+id
            listaCorTemp[pos] = corOriginal
            if(episodiosEncontradosAnime[pos] === "1")
                listaCorEpisodiosTemp[pos] = corEpisodioEncontrado
            else if(episodiosEncontradosAnime[pos] === "-1")
                listaCorEpisodiosTemp[pos] = corEpisodioNaoEncontrado
            else
                listaCorEpisodiosTemp[pos] = corTextoNormal
        }
        else{
            listaNomeTemp[pos] = ""
            listaEpisodiosTemp[pos] = ""
            listaNotaTemp[pos] = ""
            listaListaTemp[pos] = ""
            listaImaemTemp[pos] = ""
            listaCorTemp[pos] = "transparent"
        }
        localNome = listaNomeTemp
        localEpisodios = listaEpisodiosTemp
        localNota = listaNotaTemp
        localLista = listaListaTemp
        localCor = listaCorTemp
        localImagem = listaImaemTemp
        localCorEpisodios = listaCorEpisodiosTemp
    }

    Column {
        id: column12
        spacing: 2
        anchors.fill: parent

        Row {
            id: row
            width: 268
            height: 150
            spacing: 8

            Column {
                id: column
                width: 260
                height: 150
                spacing: 0

                Item {
                    id: element2
                    width: 260
                    height: 150

                    AnimeSlot {
                        id: animeSlot
                        width: 260
                        height: 150
                        posicao: 0
                        idAnimeSlot: idAnime[posicao]
                        onIdAnimeSlotChanged: getDataFromMain(idAnimeSlot, posicao)
                        titulo: localNome[posicao]
                        episodios: localEpisodios[posicao]
                        nota: localNota[posicao]
                        lista: localLista[posicao]
                        corLabel: localCor[posicao]
                        imagem: localImagem[posicao]
                        corEpisodio: localCorEpisodios[posicao]

                    }
                }
            }

            Column {
                id: column1
                width: 260
                height: 150
                spacing: 0

                Item {
                    id: element1
                    width: 260
                    height: 150

                    AnimeSlot {
                        id: animeSlot1
                        x: 0
                        y: 0
                        width: 260
                        height: 150
                        anchors.fill: parent
                        posicao: 1
                        idAnimeSlot: idAnime[posicao]
                        onIdAnimeSlotChanged: getDataFromMain(idAnimeSlot, posicao)
                        titulo: localNome[posicao]
                        episodios: localEpisodios[posicao]
                        nota: localNota[posicao]
                        lista: localLista[posicao]
                        corLabel: localCor[posicao]
                        imagem: localImagem[posicao]
                        corEpisodio: localCorEpisodios[posicao]


                    }
                }
            }

            Column {
                id: column2
                width: 260
                height: 150
                spacing: 0

                Item {
                    id: element3
                    width: 260
                    height: 150

                    AnimeSlot {
                        id: animeSlot2
                        x: 0
                        y: 0
                        anchors.fill: parent
                        posicao: 2
                        idAnimeSlot: idAnime[posicao]
                        onIdAnimeSlotChanged: getDataFromMain(idAnimeSlot, posicao)
                        titulo: localNome[posicao]
                        episodios: localEpisodios[posicao]
                        nota: localNota[posicao]
                        lista: localLista[posicao]
                        corLabel: localCor[posicao]
                        imagem: localImagem[posicao]
                        corEpisodio: localCorEpisodios[posicao]


                    }
                }
            }
        }

        Row {
            id: row1
            width: 268
            height: 150
            spacing: 8
            Column {
                id: column3
                width: 260
                height: 150
                spacing: 0

                Item {
                    id: element4
                    width: 260
                    height: 150

                    AnimeSlot {
                        id: animeSlot3
                        x: 0
                        y: 0
                        width: 260
                        height: 150
                        posicao: 3
                        idAnimeSlot: idAnime[posicao]
                        onIdAnimeSlotChanged: getDataFromMain(idAnimeSlot, posicao)
                        episodios: localEpisodios[posicao]
                        corLabel: localCor[posicao]
                        titulo: localNome[posicao]
                        lista: localLista[posicao]
                        nota: localNota[posicao]
                        imagem: localImagem[posicao]
                        corEpisodio: localCorEpisodios[posicao]

                    }
                }
            }

            Column {
                id: column4
                width: 260
                height: 150
                spacing: 0

                Item {
                    id: element5
                    width: 260
                    height: 150

                    AnimeSlot {
                        id: animeSlot4
                        x: 0
                        y: 0
                        posicao: 4
                        idAnimeSlot: idAnime[posicao]
                        onIdAnimeSlotChanged: getDataFromMain(idAnimeSlot, posicao)
                        episodios: localEpisodios[posicao]
                        anchors.fill: parent
                        corLabel: localCor[posicao]
                        titulo: localNome[posicao]
                        lista: localLista[posicao]
                        nota: localNota[posicao]
                        imagem: localImagem[posicao]
                        corEpisodio: localCorEpisodios[posicao]

                    }
                }
            }

            Column {
                id: column5
                width: 260
                height: 150
                spacing: 0

                Item {
                    id: element6
                    width: 260
                    height: 150

                    AnimeSlot {
                        id: animeSlot5
                        x: 0
                        y: 0
                        posicao: 5
                        idAnimeSlot: idAnime[posicao]
                        onIdAnimeSlotChanged: getDataFromMain(idAnimeSlot, posicao)
                        episodios: localEpisodios[posicao]
                        anchors.fill: parent
                        corLabel: localCor[posicao]
                        titulo: localNome[posicao]
                        lista: localLista[posicao]
                        nota: localNota[posicao]
                        imagem: localImagem[posicao]
                        corEpisodio: localCorEpisodios[posicao]

                    }
                }
            }
        }

        Row {
            id: row2
            width: 268
            height: 150
            spacing: 8
            Column {
                id: column6
                width: 260
                height: 150
                spacing: 0

                Item {
                    id: element7
                    width: 260
                    height: 150

                    AnimeSlot {
                        id: animeSlot6
                        x: 0
                        y: 0
                        width: 260
                        height: 150
                        posicao: 6
                        idAnimeSlot: idAnime[posicao]
                        onIdAnimeSlotChanged: getDataFromMain(idAnimeSlot, posicao)
                        episodios: localEpisodios[posicao]
                        corLabel: localCor[posicao]
                        titulo: localNome[posicao]
                        lista: localLista[posicao]
                        nota: localNota[posicao]
                        imagem: localImagem[posicao]
                        corEpisodio: localCorEpisodios[posicao]

                    }
                }
            }

            Column {
                id: column7
                width: 260
                height: 150
                spacing: 0

                Item {
                    id: element8
                    width: 260
                    height: 150

                    AnimeSlot {
                        id: animeSlot7
                        x: 0
                        y: 0
                        posicao: 7
                        idAnimeSlot: idAnime[posicao]
                        onIdAnimeSlotChanged: getDataFromMain(idAnimeSlot, posicao)
                        episodios: localEpisodios[posicao]
                        anchors.fill: parent
                        corLabel: localCor[posicao]
                        titulo: localNome[posicao]
                        lista: localLista[posicao]
                        nota: localNota[posicao]
                        imagem: localImagem[posicao]
                        corEpisodio: localCorEpisodios[posicao]

                    }
                }
            }

            Column {
                id: column8
                width: 260
                height: 150
                spacing: 0

                Item {
                    id: element9
                    width: 260
                    height: 150

                    AnimeSlot {
                        id: animeSlot8
                        x: 0
                        y: 0
                        posicao: 8
                        idAnimeSlot: idAnime[posicao]
                        onIdAnimeSlotChanged: getDataFromMain(idAnimeSlot, posicao)
                        episodios: localEpisodios[posicao]
                        anchors.fill: parent
                        corLabel: localCor[posicao]
                        titulo: localNome[posicao]
                        lista: localLista[posicao]
                        nota: localNota[posicao]
                        imagem: localImagem[posicao]
                        corEpisodio: localCorEpisodios[posicao]

                    }
                }
            }
        }

        Row {
            id: row3
            width: 268
            height: 150
            spacing: 8
            Column {
                id: column9
                width: 260
                height: 150
                spacing: 0

                Item {
                    id: element10
                    width: 260
                    height: 150

                    AnimeSlot {
                        id: animeSlot9
                        x: 0
                        y: 0
                        width: 260
                        height: 150
                        posicao: 9
                        idAnimeSlot: idAnime[posicao]
                        onIdAnimeSlotChanged: getDataFromMain(idAnimeSlot, posicao)
                        episodios: localEpisodios[posicao]
                        corLabel: localCor[posicao]
                        titulo: localNome[posicao]
                        lista: localLista[posicao]
                        nota: localNota[posicao]
                        imagem: localImagem[posicao]
                        corEpisodio: localCorEpisodios[posicao]

                    }
                }
            }

            Column {
                id: column10
                width: 260
                height: 150
                spacing: 0

                Item {
                    id: element11
                    x: 0
                    width: 260
                    height: 150

                    AnimeSlot {
                        id: animeSlot10
                        x: 0
                        y: 0
                        posicao: 10
                        idAnimeSlot: idAnime[posicao]
                        onIdAnimeSlotChanged: getDataFromMain(idAnimeSlot, posicao)
                        episodios: localEpisodios[posicao]
                        anchors.fill: parent
                        corLabel: localCor[posicao]
                        titulo: localNome[posicao]
                        lista: localLista[posicao]
                        nota: localNota[posicao]
                        imagem: localImagem[posicao]
                        corEpisodio: localCorEpisodios[posicao]

                    }
                }
            }

            Column {
                id: column11
                width: 260
                height: 150
                spacing: 0

                Item {
                    id: element12
                    width: 260
                    height: 150

                    AnimeSlot {
                        id: animeSlot11
                        x: 0
                        y: 0
                        posicao: 11
                        anchors.fill: parent
                        idAnimeSlot: idAnime[posicao]
                        onIdAnimeSlotChanged: getDataFromMain(idAnimeSlot, posicao)
                        episodios: localEpisodios[posicao]
                        titulo: localNome[posicao]
                        lista: localLista[posicao]
                        nota: localNota[posicao]
                        corLabel: localCor[posicao]
                        imagem: localImagem[posicao]
                        corEpisodio: localCorEpisodios[posicao]

                    }
                }
            }

        }

    }

}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.8999999761581421}D{i:5;anchors_height:150;anchors_width:268}
D{i:3;anchors_height:150;anchors_width:260}D{i:6;anchors_height:150;anchors_width:260}
D{i:11;anchors_height:150;anchors_width:804;anchors_x:0;anchors_y:0}D{i:9;anchors_height:150;anchors_width:260}
D{i:2;anchors_height:150;anchors_width:268}D{i:15;anchors_height:150;anchors_width:268}
D{i:13;anchors_height:150;anchors_width:260}D{i:18;anchors_height:150;anchors_width:268;anchors_x:536;anchors_y:0}
D{i:16;anchors_height:150;anchors_width:260}D{i:21;anchors_height:150;anchors_width:804;anchors_x:0;anchors_y:0}
D{i:19;anchors_height:150;anchors_width:260}D{i:12;anchors_height:150;anchors_width:268}
D{i:25;anchors_height:150;anchors_width:268}D{i:23;anchors_height:150;anchors_width:260}
D{i:28;anchors_height:150;anchors_width:268}D{i:26;anchors_height:150;anchors_width:260}
D{i:31;anchors_height:150;anchors_width:804;anchors_x:0;anchors_y:0}D{i:29;anchors_height:150;anchors_width:260}
D{i:22;anchors_height:150;anchors_width:268}D{i:35;anchors_height:150;anchors_width:268}
D{i:33;anchors_height:150;anchors_width:260}D{i:38;anchors_height:150;anchors_width:268}
D{i:36;anchors_height:150;anchors_width:260}D{i:41;anchors_height:150;anchors_width:804;anchors_x:0;anchors_y:0}
D{i:39;anchors_height:150;anchors_width:260}D{i:32;anchors_height:150;anchors_width:268}
D{i:1;anchors_height:400;anchors_width:200}
}
##^##*/
