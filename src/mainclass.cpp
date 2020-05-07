#include "mainclass.h"
#define vlista(x) QString(vmetaEnum.valueToKey(x))

//MAIN CLASS é a classe que se comunica com QML

MainClass::MainClass(QObject *parent) : QObject(parent)
{
    vmetaEnum = QMetaEnum::fromType<lista>();
    //Cria todas as variáveis
    cleitorListaAnimes = new leitorlistaanimes(this);
    cconfiguracoesDiretoriosPadrao = new confBase(this);
    carquivos = new arquivos(this);
    cconfiguracoesUsuarioDiretorios = new confUsuario(this);
    cclient = new Client(this);

    cclient->fselecionaClient("Anilist");
    //cclient->frecebeAutorizacao(configurações->user, configurações->codigo)
    cclient->frecebeAutorizacao("gutocbs", "codigo");


    cleitorListaAnimes->fleJson();
    vlistaSelecionada = cleitorListaAnimes->retornaListaWatching();

    cconfiguracoesDiretoriosPadrao->fcriaDiretoriosBase();
    cconfiguracoesUsuarioDiretorios->flePastasArquivos();

    carquivos->frecebeAnimes(cleitorListaAnimes);
    carquivos->frecebeDiretorios(cconfiguracoesUsuarioDiretorios);

    vindexAnimeSelecionado = 0;
    vpagina = 1;
    vlistaAtual = vlista(lista::CURRENT);
    vtipoAtual = leitorlistaanimes::type::ANIME;
    vordemLista = "";
}

void MainClass::finfoAnimeSelecionado(QVariant posicaoAnimeNaGrid)
{
    emit sdirImagensMedias(QVariant(cconfiguracoesDiretoriosPadrao->vdiretorioImagensMedio));
    emit sdirImagensGrandes(QVariant(cconfiguracoesDiretoriosPadrao->vdiretorioImagensGrandes));

    bool ok;
    vposicaoGridAnimeSelecionado = posicaoAnimeNaGrid.toInt(&ok);
    if(ok)
        vindexAnimeSelecionado = 12*(vpagina-1)+vposicaoGridAnimeSelecionado;

    if(vlistaSelecionada.size() > vindexAnimeSelecionado){
        emit snomeAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vnome));
        emit snomeAlternativoAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vnomeIngles));
        emit ssinopseAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vsinopse));
        emit sstatusAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vstatus));
        emit sseasonAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vseason));
        emit smediaGloballAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaSite));
        emit smediaPessoalAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaPessoal));
        emit sreleaseAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vdataEpisodio));
        if(vlistaSelecionada[vindexAnimeSelecionado]->vnumProximoEpisodioLancado == "" &&
                vlistaSelecionada[vindexAnimeSelecionado]->vstatus != "Finished Airing")
            emit sepisodiosLancadosAnimeSelecionado(QVariant("-"));
        else if(vlistaSelecionada[vindexAnimeSelecionado]->vnumProximoEpisodioLancado.toInt() == 0)
            emit sepisodiosLancadosAnimeSelecionado(QVariant("All Episodes"));
        else
            emit sepisodiosLancadosAnimeSelecionado(QVariant(
                QString::number(vlistaSelecionada[vindexAnimeSelecionado]->vnumProximoEpisodioLancado.toInt() - 1) + " Episodes"));
        emit stipoAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vformato));
        QPixmap pix;
        if(pix.load(cconfiguracoesDiretoriosPadrao->vdiretorioImagensGrandes+vlistaSelecionada[vindexAnimeSelecionado]->vid+".png",
                         "png"))
            emit simagemAnimeSelecionado(QVariant(cconfiguracoesDiretoriosPadrao->vdiretorioImagensGrandes+
                                                  vlistaSelecionada[vindexAnimeSelecionado]->vid+".png"));
        else if(pix.load(cconfiguracoesDiretoriosPadrao->vdiretorioImagensGrandes+vlistaSelecionada[vindexAnimeSelecionado]->vid+".jpg", "jpg"))
            emit simagemAnimeSelecionado(QVariant(cconfiguracoesDiretoriosPadrao->vdiretorioImagensGrandes+
                                                  vlistaSelecionada[vindexAnimeSelecionado]->vid+".jpg"));
        else
            emit simagemAnimeSelecionado(QVariant(cconfiguracoesDiretoriosPadrao->vdiretorioImagensMedio+
                                     vlistaSelecionada[vindexAnimeSelecionado]->vid));
    }
    emit sidAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vid));
    fmostraListaAnimes();
}

QVariant MainClass::fretornaNomeAnimePosicao(QVariant posicao)
{
    if(vlistaSelecionada.size() > (12*(vpagina-1))+posicao.toInt())
        return QVariant(vlistaSelecionada[(12*(vpagina-1))+posicao.toInt()]->vnome);
    return QVariant("");
}
QVariant MainClass::fretornaEpisodiosAnimePosicao(QVariant posicao)
{
    if(vlistaSelecionada.size() > (12*(vpagina-1))+posicao.toInt())
        return QVariant("Progresso: " + vlistaSelecionada[(12*(vpagina-1))+posicao.toInt()]->vnumEpisodiosAssistidos +
                                        "/" + vlistaSelecionada[(12*(vpagina-1))+posicao.toInt()]->vnumEpisodiosTotais);
    return QVariant("");
}

QVariant MainClass::fretornaNotaAnimePosicao(QVariant posicao)
{
    if(vlistaSelecionada.size() > (12*(vpagina-1))+posicao.toInt())
        return QVariant("Nota: " + vlistaSelecionada[(12*(vpagina-1))+posicao.toInt()]->vnotaMediaPessoal + "/10");
    return QVariant("");
}

QVariant MainClass::fretornaListaAnimePosicao(QVariant posicao)
{
    if(vlistaSelecionada.size() > (12*(vpagina-1))+posicao.toInt())
        return QVariant(vlistaSelecionada[(12*(vpagina-1))+posicao.toInt()]->vlista);
    return QVariant("");
}

void MainClass::fordemLista(QVariant ordem)
{
    //First we need to see if the order is the same, and if it is, we will change the first letter
    //To invert the order
    if(vordemLista.contains(ordem.toString()) && vordemLista.front() == "c")
        vordemLista.replace(0,1,"v");
    else if(vordemLista.contains(ordem.toString()) && vordemLista.front() == "v")
        vordemLista.replace(0,1,"c");
    //If it's not, let's change to the new order, with a C to make it from low to high
    else
        vordemLista = QString("c" + ordem.toString());

    //Change the old list to the new
    vlistaSelecionada = cleitorListaAnimes->sortLista(vordemLista, vlistaAtual, vtipoAtual);
    //Check if the list is empty for some reason
    if(!vlistaSelecionada.isEmpty()){
        vindexAnimeSelecionado = 0;
        vpagina = 1;
        finfoAnimeSelecionado(0);
    }
}

void MainClass::fabreSite(QVariant data)
{
    bool ok;
    data.toInt(&ok);
    if(ok){
        QDesktopServices::openUrl(QUrl(vlistaSelecionada[vindexAnimeSelecionado]->vsiteAnilist));
    }
}

void MainClass::fselecionaListaCurrent()
{
    //Check if it is already the right list
    if(vlistaAtual.compare(vlista(lista::CURRENT), Qt::CaseInsensitive) != 0){
        vlistaAtual = vlista(lista::CURRENT);
        //Change the old list to the new
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordemLista, vlistaAtual, vtipoAtual);
        //Check if the list is empty for some reason
        if(!vlistaSelecionada.isEmpty()){
            vindexAnimeSelecionado = 0;
            vpagina = 1;
            finfoAnimeSelecionado(0);
        }
    }
}

void MainClass::fselecionaListaCompleted()
{
    //Check if it is already the right list
    if(vlistaAtual.compare(vlista(lista::COMPLETED), Qt::CaseInsensitive) != 0){
        vlistaAtual = vlista(lista::COMPLETED);
        //Change the old list to the new
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordemLista, vlistaAtual, vtipoAtual);
        //Check if the list is empty for some reason
        if(!vlistaSelecionada.isEmpty()){
            vindexAnimeSelecionado = 0;
            vpagina = 1;
            finfoAnimeSelecionado(0);
        }
    }
}

void MainClass::fselecionaListaPaused()
{
    //Check if it is already the right list
    if(vlistaAtual.compare(vlista(lista::PAUSED), Qt::CaseInsensitive) != 0){
        vlistaAtual = vlista(lista::PAUSED);
        //Change the old list to the new
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordemLista, vlistaAtual, vtipoAtual);
        //Check if the list is empty for some reason
        if(!vlistaSelecionada.isEmpty()){
            vindexAnimeSelecionado = 0;
            vpagina = 1;
            finfoAnimeSelecionado(0);
        }
    }
}

void MainClass::fselecionaListaDropped()
{
    //Check if it is already the right list
    if(vlistaAtual.compare(vlista(lista::DROPPED), Qt::CaseInsensitive) != 0){
        vlistaAtual = vlista(lista::DROPPED);
        //Change the old list to the new
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordemLista, vlistaAtual, vtipoAtual);
        //Check if the list is empty for some reason
        if(!vlistaSelecionada.isEmpty()){
            vindexAnimeSelecionado = 0;
            vpagina = 1;
            finfoAnimeSelecionado(0);
        }
    }
}

void MainClass::fselecionaListaPlanning()
{
    //Check if it is already the right list
    if(vlistaAtual.compare(vlista(lista::PLANNING), Qt::CaseInsensitive) != 0){
        vlistaAtual = vlista(lista::PLANNING);
        //Change the old list to the new
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordemLista, vlistaAtual, vtipoAtual);
        //Check if the list is empty for some reason
        if(!vlistaSelecionada.isEmpty()){
            vindexAnimeSelecionado = 0;
            vpagina = 1;
            finfoAnimeSelecionado(0);
        }
    }
}


///TODO: MAKE WARNINGS
void MainClass::fabreProximoEpisodio()
{
    //Search for the next episode in the computer. If it exists, a string with it's path will be returned.
    //Otherwise, will be an empty string.
    QString episodePath = carquivos->fprocuraEpisodio(vlistaSelecionada[vindexAnimeSelecionado]);
    if(!episodePath.isEmpty()){
        //Try to open the file and return true, if it can
        if(carquivos->fabreEpisodio(episodePath.toUtf8()))
            return;
    }
}

void MainClass::fabrePastaAnime()
{
    QByteArray lcaminhoPasta = cconfiguracoesUsuarioDiretorios->fretornaDiretorioEspecifico(
                vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt()).toUtf8();
    if(!lcaminhoPasta.isEmpty())
        carquivos->fabreEpisodio(lcaminhoPasta);
}

void MainClass::fabreStream()
{
    QDesktopServices::openUrl(QUrl(vlistaSelecionada[vindexAnimeSelecionado]->vstreamCrunchyroll));
}

void MainClass::fmostraListaAnimes()
{
    if(vlistaSelecionada.size() > 0+(12*(vpagina-1)))
        emit sidAnime1(QVariant(vlistaSelecionada[(12*(vpagina-1))]->vid));
    else
        emit sidAnime1(QVariant("null"));
    if(vlistaSelecionada.size() > (12*(vpagina-1))+1)
        emit sidAnime2(QVariant(vlistaSelecionada[(12*(vpagina-1))+1]->vid));
    else
        emit sidAnime2(QVariant("null"));
    if(vlistaSelecionada.size() > (12*(vpagina-1))+2)
        emit sidAnime3(QVariant(vlistaSelecionada[(12*(vpagina-1))+2]->vid));
    else
        emit sidAnime3(QVariant("null"));
    if(vlistaSelecionada.size() > (12*(vpagina-1))+3)
        emit sidAnime4(QVariant(vlistaSelecionada[(12*(vpagina-1))+3]->vid));
    else
        emit sidAnime4(QVariant("null"));
    if(vlistaSelecionada.size() > (12*(vpagina-1))+4)
        emit sidAnime5(QVariant(vlistaSelecionada[(12*(vpagina-1))+4]->vid));
    else
        emit sidAnime5(QVariant("null"));
    if(vlistaSelecionada.size() > (12*(vpagina-1))+5)
        emit sidAnime6(QVariant(vlistaSelecionada[(12*(vpagina-1))+5]->vid));
    else
        emit sidAnime6(QVariant("null"));
    if(vlistaSelecionada.size() > (12*(vpagina-1))+6)
        emit sidAnime7(QVariant(vlistaSelecionada[(12*(vpagina-1))+6]->vid));
    else
        emit sidAnime7(QVariant("null"));
    if(vlistaSelecionada.size() > (12*(vpagina-1))+7)
        emit sidAnime8(QVariant(vlistaSelecionada[(12*(vpagina-1))+7]->vid));
    else
        emit sidAnime8(QVariant("null"));
    if(vlistaSelecionada.size() > (12*(vpagina-1))+8)
        emit sidAnime9(QVariant(vlistaSelecionada[(12*(vpagina-1))+8]->vid));
    else
        emit sidAnime9(QVariant("null"));
    if(vlistaSelecionada.size() > (12*(vpagina-1))+9)
        emit sidAnime10(QVariant(vlistaSelecionada[(12*(vpagina-1))+9]->vid));
    else
        emit sidAnime10(QVariant("null"));
    if(vlistaSelecionada.size() > (12*(vpagina-1))+10)
        emit sidAnime11(QVariant(vlistaSelecionada[(12*(vpagina-1))+10]->vid));
    else
        emit sidAnime11(QVariant("null"));
    if(vlistaSelecionada.size() > (12*(vpagina-1))+11)
        emit sidAnime12(QVariant(vlistaSelecionada[(12*(vpagina-1))+11]->vid));
    else
        emit sidAnime12(QVariant("null"));
}


void MainClass::fproximaPagina()
{
    if(vlistaSelecionada.size() > 12+(12*(vpagina-1))){
        vpagina++;
        fmostraListaAnimes();
    }
}

void MainClass::fanteriorPagina()
{
    if(vpagina > 1){
        vpagina--;
        fmostraListaAnimes();
    }
}

void MainClass::fmudaListaAnime(QVariant rnewListVariant)
{
    QString rnewList = rnewListVariant.toString();
    //Check if the user is trying to change the entry from list A to list A
    if(rnewList.compare(vlistaAtual, Qt::CaseInsensitive) == 0)
        return;
    switch (vtipoAtual) {
    case leitorlistaanimes::type::ANIME:
        if(rnewList.compare("CURRENT") == 0){
            //Change the anime info
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Watching";
            //Update anime client
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "CURRENT");
            //Update local variables
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("COMPLETED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Completed";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "COMPLETED");
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PAUSED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "On Hold";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PAUSED");
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("DROPPED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Dropped";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "DROPPED");
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PLANNING") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Plan to Watch";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PLANNING");
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        break;
    case leitorlistaanimes::type::MANGA:
        if(rnewList.compare("CURRENT") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Reading(Manga)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "CURRENT");
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("COMPLETED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Completed(Manga)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "COMPLETED");
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PAUSED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "On Hold(Manga)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PAUSED");
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("DROPPED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Dropped(Manga)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "DROPPED");
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PLANNING") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Plan to Read(Manga)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PLANNING");
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        break;
    case leitorlistaanimes::type::NOVEL:
        if(rnewList.compare("CURRENT") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Reading(Novel)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "CURRENT");
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("COMPLETED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Completed(Novel)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "COMPLETED");
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PAUSED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "On Hold(Novel)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PAUSED");
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("DROPPED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Dropped(Novel)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "DROPPED");
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PLANNING") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Plan to Read(Novel)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PLANNING");
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        break;
    }

    //Update de GUI with the new, change, lists
    vlistaSelecionada = cleitorListaAnimes->sortLista(vordemLista, vlistaAtual, vtipoAtual);
    if(vlistaSelecionada.size() > vindexAnimeSelecionado && vindexAnimeSelecionado != 0){
        vindexAnimeSelecionado--;
        if(vindexAnimeSelecionado < 12*(vpagina-1) && vpagina != 1){
            vpagina--;
        }
    }
    else{
        vindexAnimeSelecionado = 0;
        vpagina = 1;
    }
    finfoAnimeSelecionado(vposicaoGridAnimeSelecionado);
}
