import QtQuick 2.12
import QtQuick.Window 2.15
import QtQuick.Controls 2.3

Window {
    id: root
    visible: true
//    property var tamanhoJanela: ScreenInfo.width
    width: 1920
    height: 1080*(1920/1920)


    visibility: "Maximized"
    color: "#9caef0"
    title: qsTr("Haiken anime")

    property var escala: width/1920

    //Configurações de diretorios
    property var dirImagensGrandes: ""
    property var dirImagensMedias: ""

    //Id dos animes
    property var idAnime: ["null","null","null","null","null","null","null","null","null","null","null","null"]
    //Nome dos Animes
    property var nomeAnime: ["0","1","2","3","4","5","6","7","8","9","10","11"]
    //Episodios
    property var episodiosAnime: ["0","1","2","3","4","5","6","7","8","9","10","11"]
    property var episodiosEncontradosAnime: ["0","0","0","0","0","0","0","0","0","0","0","0"]
    //Nota
    property var notaAnime: ["0","1","2","3","4","5","6","7","8","9","10","11"]
    //Lista
    property var listaAnime: ["0","1","2","3","4","5","6","7","8","9","10","11"]
    //Anime AnimeSelecionado
    property var idAnimeSelecionado: "null"
    property var nomeAnimeSelecionado: "Peter Pan"
    property var nomeAlternativoAnimeSelecionado: "Peter Pan"
    property var sinopseAnimeSelecionado: "Peter Pan"
    property var statusAnimeSelecionado: "Peter Pan"
    property var seasonAnimeSelecionado: "Peter Pan"
    property var mediaGlobalAnimeSelecionado: "Peter Pan"
    property var mediaPessoalAnimeSelecionado: "Peter Pan"
    property var releaseAnimeSelecionado: "Peter Pan"
    property var episodiosAssistidosAnimeSelecionado: "Peter Pan"
    property var episodiosTotaisAnimeSelecionado: "Peter Pan"
    property var episodiosAnimeSelecionado: "Peter Pan"
    property var tipoAnimeSelecionado: "Peter Pan"
    property var imagemAnimeSelecionado: "null"
    property var proximoEpisodioAnimeSelecionado: "#1bd36a"
    property var imagemAvatar: ""
    property var nomeUsuario: ""
    property var time: ""
    //Anime reconhecido
    property var idanimeReconhecido: ""
    property var nomeAnimeReconhecido: ""
    property var episodiosAnimeReconhecido: ""


    property var numeroAnosListas: 0

    property var configSelecionada: ""


    function getData(id, posicaoGrid){
        var listaIdTemp = idAnime
        var listaNomeTemp = nomeAnime
        var listaEpisodiosTemp = episodiosAnime
        var listaNotaTemp = notaAnime
        var listaListaTemp = listaAnime
        var listaEpisodiosEncontradosTemp = episodiosEncontradosAnime
        listaIdTemp[posicaoGrid] = id
        listaNomeTemp[posicaoGrid] = mainClass.fretornaNomeAnimePosicao(posicaoGrid)
        listaEpisodiosTemp[posicaoGrid] = mainClass.fretornaEpisodiosAnimePosicao(posicaoGrid)
        listaNotaTemp[posicaoGrid] = mainClass.fretornaNotaAnimePosicao(posicaoGrid)
        if(mainClass.fretornaEpisodioAnimeEncontrado(posicaoGrid) === "1")
            listaEpisodiosEncontradosTemp[posicaoGrid] = "1"
        else if(mainClass.fretornaEpisodioAnimeEncontrado(posicaoGrid) === "-1")
            listaEpisodiosEncontradosTemp[posicaoGrid] = "-1"
        else
            listaEpisodiosEncontradosTemp[posicaoGrid] = "0"
        idAnime = listaIdTemp
        nomeAnime = listaNomeTemp
        episodiosAnime = listaEpisodiosTemp
        notaAnime = listaNotaTemp
        listaAnime = listaListaTemp
        episodiosEncontradosAnime = listaEpisodiosEncontradosTemp

    }

    Connections{
        target: mainClass

        function onSdirImagensMedias(data){ dirImagensMedias = data}
        function onSdirImagensGrandes(data){ dirImagensGrandes = data}
        function onSidAnime1(data){ getData(data,0)}
        function onSidAnime2(data){ getData(data,1)}
        function onSidAnime3(data){ getData(data,2)}
        function onSidAnime4(data){ getData(data,3)}
        function onSidAnime5(data){ getData(data,4)}
        function onSidAnime6(data){ getData(data,5)}
        function onSidAnime7(data){ getData(data,6)}
        function onSidAnime8(data){ getData(data,7)}
        function onSidAnime9(data){ getData(data,8)}
        function onSidAnime10(data){ getData(data,9)}
        function onSidAnime11(data){ getData(data,10)}
        function onSidAnime12(data){ getData(data,11)}

        function onSidAnimeSelecionado(data){ idAnimeSelecionado = data}
        function onSnomeAnimeSelecionado(data){ nomeAnimeSelecionado = data}
        function onSnomeAlternativoAnimeSelecionado(data){ nomeAlternativoAnimeSelecionado = data}
        function onSsinopseAnimeSelecionado(data){ sinopseAnimeSelecionado = data}
        function onSstatusAnimeSelecionado(data){ statusAnimeSelecionado = data}
        function onSseasonAnimeSelecionado(data){ seasonAnimeSelecionado = data}
        function onSmediaGloballAnimeSelecionado(data){ mediaGlobalAnimeSelecionado = data}
        function onSmediaPessoalAnimeSelecionado(data){ mediaPessoalAnimeSelecionado = data}
        function onSreleaseAnimeSelecionado(data){ releaseAnimeSelecionado = data}
        function onSepisodiosLancadosAnimeSelecionado(data){ episodiosAnimeSelecionado = data}
        function onSepisodiosTotaisAnimeSelecionado(data){ episodiosTotaisAnimeSelecionado = data}
        function onSepisodiosAssistidosAnimeSelecionado(data){ episodiosAssistidosAnimeSelecionado = data}
        function onStipoAnimeSelecionado(data){ tipoAnimeSelecionado = data}
        function onSproximoEpisodioAnimeSelecionado(data){
            if(data)
                proximoEpisodioAnimeSelecionado = "#1bd36a"
            else
                proximoEpisodioAnimeSelecionado = "#d31b1b"
        }

        function onSimagemAnimeSelecionado(data){ imagemAnimeSelecionado = data}
        function onStimer(data){ time = data}
        function onSconnectGUI(data){
            if(data === false){
                slotMenus.enabled = false;
                stackView.enabled = false
            }
            else{
                slotMenus.enabled = true;
                stackView.enabled = true;
            }
        }
        function onSbotaoHome(data){ stackView.push("Home.qml")}
        function onSbotaoConfig(data){ stackView.push("ConfigPagina.qml")}
        function onSbotaoTorrent(data){ stackView.push("TorrentPagina.qml")}
        function onSanimeReconhecidoID(data){
            idanimeReconhecido = dataId
            nomeAnimeReconhecido = dataNome
            episodiosAnimeReconhecido = dataEpisodio
        }
    }

    Component.onCompleted: {
        mainClass.finfoAnimeSelecionado(0)
        numeroAnosListas = mainClass.fretornaNumeroAnos()
        imagemAvatar = "file:///"+mainClass.fretornaPathAvatar()
        nomeUsuario = mainClass.fretornaNomeUsuario()
    }

    Column {
        id: column
        anchors.fill: parent
        transformOrigin: Item.TopLeft
        scale: escala

        Row {
            id: row
            width: parent.width
            height: 187
            topPadding: 0
            leftPadding: 8
            spacing: 14

            SlotMenus {
                id: slotMenus
                y: 0
                width: 1912
            }
        }

        StackView {
            id: stackView
            width: 1920
            height: parent.height-187
            initialItem: "Home.qml"
            pushEnter: Transition {
                PropertyAnimation {
                    property: "opacity"
                    from: 0
                    to:1
                    duration: 200
                }
            }
            pushExit: Transition {
                PropertyAnimation {
                    property: "opacity"
                    from: 1
                    to:0
                    duration: 200
                }
            }
        }

//        Row {
//            id: row1
//            width: parent.width
//            height: parent.height-187
//                Home{
//                    id: home
//                    x: 0
//                    y: 0
//                    transformOrigin: Item.Center;
//                }
//        }

    }

}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.25}D{i:1}D{i:2}
}
##^##*/
