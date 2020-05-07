import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.3

Window {
    id: root
    visible: true
    width: 1920
    height: 1080
    visibility: "Maximized"
    color: "#9caef0"
    title: qsTr("Hello World")

    //Configurações de diretorios
    property var dirImagensGrandes: ""
    property var dirImagensMedias: ""

    //Id dos animes
    property var idAnime: ["null","null","null","null","null","null","null","null","null","null","null","null"]
    //Nome dos Animes
    property var nomeAnime: ["0","1","2","3","4","5","6","7","8","9","10","11"]
    //Episodios
    property var episodiosAnime: ["0","1","2","3","4","5","6","7","8","9","10","11"]
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
    property var episodiosAnimeSelecionado: "Peter Pan"
    property var tipoAnimeSelecionado: "Peter Pan"
    property var imagemAnimeSelecionado: "null"

    function getData(id, posicaoGrid){
        var listaIdTemp = idAnime
        var listaNomeTemp = nomeAnime
        var listaEpisodiosTemp = episodiosAnime
        var listaNotaTemp = notaAnime
        var listaListaTemp = listaAnime
        listaIdTemp[posicaoGrid] = id
        listaNomeTemp[posicaoGrid] = mainClass.fretornaNomeAnimePosicao(posicaoGrid)
        listaEpisodiosTemp[posicaoGrid] = mainClass.fretornaEpisodiosAnimePosicao(posicaoGrid)
        listaNotaTemp[posicaoGrid] = mainClass.fretornaNotaAnimePosicao(posicaoGrid)
        listaListaTemp[posicaoGrid] = mainClass.fretornaListaAnimePosicao(posicaoGrid)
        idAnime = listaIdTemp
        nomeAnime = listaNomeTemp
        episodiosAnime = listaEpisodiosTemp
        notaAnime = listaNotaTemp
        listaAnime = listaListaTemp
    }

    Connections{
        target: mainClass

        onSdirImagensMedias: dirImagensMedias = data
        onSdirImagensGrandes: dirImagensGrandes = data
        onSidAnime1: getData(data,0)
        onSidAnime2: getData(data,1)
        onSidAnime3: getData(data,2)
        onSidAnime4: getData(data,3)
        onSidAnime5: getData(data,4)
        onSidAnime6: getData(data,5)
        onSidAnime7: getData(data,6)
        onSidAnime8: getData(data,7)
        onSidAnime9: getData(data,8)
        onSidAnime10: getData(data,9)
        onSidAnime11: getData(data,10)
        onSidAnime12: getData(data,11)

        onSidAnimeSelecionado: idAnimeSelecionado = data
        onSnomeAnimeSelecionado: nomeAnimeSelecionado = data
        onSnomeAlternativoAnimeSelecionado: nomeAlternativoAnimeSelecionado = data
        onSsinopseAnimeSelecionado: sinopseAnimeSelecionado = data
        onSstatusAnimeSelecionado: statusAnimeSelecionado = data
        onSseasonAnimeSelecionado: seasonAnimeSelecionado = data
        onSmediaGloballAnimeSelecionado: mediaGlobalAnimeSelecionado = data
        onSmediaPessoalAnimeSelecionado: mediaPessoalAnimeSelecionado = data
        onSreleaseAnimeSelecionado: releaseAnimeSelecionado = data
        onSepisodiosLancadosAnimeSelecionado: episodiosAnimeSelecionado = data
        onStipoAnimeSelecionado: tipoAnimeSelecionado = data
        onSimagemAnimeSelecionado: imagemAnimeSelecionado = data
    }

    Component.onCompleted: mainClass.finfoAnimeSelecionado(0);

    Column {
        id: column
        anchors.fill: parent

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
            }
        }

        Row {
            id: row1
            width: parent.width
            height: parent.height-187

            Home{
                transformOrigin: Item.Center;
            }
        }
    }

}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.6600000262260437}D{i:1;anchors_x:0;anchors_y:266}D{i:2;anchors_height:187;anchors_width:1920}
}
##^##*/
