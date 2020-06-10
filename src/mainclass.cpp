#include "mainclass.h"
#define enumlistaToQString(x) QString(vmetaEnumLista.valueToKey(x))
#define enumtipoToQString(x) QString(vmetaEnumTipo.valueToKey(x))

//MAIN CLASS é a classe que se comunica com QML

MainClass::MainClass(QObject *parent) : QObject(parent)
{
    vmetaEnumLista = QMetaEnum::fromType<lista>();
    vmetaEnumTipo = QMetaEnum::fromType<leitorlistaanimes::type>();
    //Cria todas as variáveis
    cconfiguracoesDiretoriosPadrao = new confBase(nullptr);
    cleitorListaAnimes = new leitorlistaanimes(nullptr);
//    cleitorListaAnimes->instance();
    cconfiguracoesUsuarioDiretorios = new confUsuario(nullptr);
    carquivos = new arquivos(this);
    cclient = new Client(this);
    cdownloader = new Downloader(this);
    cabaConfig = new abaConfig(this);

    cclient->fselecionaClient("Anilist");
    //cclient->frecebeAutorizacao(configurações->user, configurações->codigo)
    cclient->frecebeAutorizacao("gutocbs", cabaConfig->instance()->fgetAuthCode());
    cclient->fbaixaListas();


    cleitorListaAnimes->instance()->fleJson();
    vlistaSelecionada = cleitorListaAnimes->instance()->retornaListaWatching();


    cconfiguracoesDiretoriosPadrao->instance()->fcriaDiretoriosBase();
    cconfiguracoesUsuarioDiretorios->instance()->flePastasArquivos();

    vposicaoGridAnimeSelecionado = 0;
    vindexAnimeSelecionado = 0;
    vpagina = 1;
    vlistaAtual = enumlistaToQString(lista::CURRENT);
    vtipoAtual = leitorlistaanimes::type::ANIME;
    vordemLista = "";

    vtimerAutoRefresh = new QTimer(this);
    vtimerCountdown = new QTimer(this);

    fconnections();
}

MainClass::~MainClass()
{
    if(tthreadDiretorios.isRunning()){
        tthreadDiretorios.requestInterruption();
        tthreadDiretorios.wait();
    }
    cconfiguracoesDiretoriosPadrao->deleteLater();
    cleitorListaAnimes->deleteLater();
    cconfiguracoesUsuarioDiretorios->deleteLater();
}

void MainClass::fconnections()
{
    //Ao terminar de baixar a lista, começa a rodar o programa
    connect(cclient, &Client::sdownloadCompleted, this, &MainClass::ftryClientConnection);
}

void MainClass::fdownloadCoverImages()
{
    if(!vlistaFilaLista.contains(vlistaAtual)){
        vlistaFilaLista.append(vlistaAtual);
        vlistaFilaTipo.append(enumtipoToQString(vtipoAtual));
        vlistaFilaSize.append(vlistaSelecionada.size());
    }

    if(cdownloader->isBusy() && !vlistaFilaTipo.isEmpty()){
        QTimer::singleShot(5000, this, &MainClass::fdownloadCoverImages);
    }
    else{
        cdownloader->setListAndType(vlistaFilaLista.takeFirst(), vlistaFilaTipo.takeFirst());
    //    cdownloader->setListAndType(vlistaFilaLista.first(), vlistaFilaTipo.first());
        for(int i = 0; i < vlistaFilaSize.first(); i++){
            cdownloader->work(i);
        }
    //    cdownloader->setListAndType(vlistaFilaLista.takeFirst(), vlistaFilaTipo.takeFirst());
        for(int i = 0; i < vlistaFilaSize.first(); i++){
            cdownloader->workBig(i);
        }
        vlistaFilaSize.removeFirst();
        if(!cdownloader->isBusy() && !vlistaFilaTipo.isEmpty())
            QTimer::singleShot(1000, this, &MainClass::fdownloadCoverImages);
    }
}

void MainClass::ftryClientConnection(bool connection)
{
    if(!connection)
        fconnectFail();
    else
        fconnectSuccess();
}

void MainClass::fconnectSuccess()
{
    ///TODO: BLOQUEAR BOTÕES
    //If the list is not empty, we have to empty it first.
    emit sconnectGUI(false);
    if(!vlistaSelecionada.isEmpty())
        cleitorListaAnimes->instance()->fdeletaListaAnimes();
    cleitorListaAnimes->instance()->fleJson();
    vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);

    //We need to check if the list got empy before reading it
    if(vlistaSelecionada.size() < vindexAnimeSelecionado){
        vindexAnimeSelecionado = 0;
        vpagina = 1;
    }
    finfoAnimeSelecionado(0);
    emit sconnectGUI(true);

    //The auto-update timer starts
    time = QTime::fromString("10","m");
    vtimerAutoRefresh->setInterval(600000);
    vtimerCountdown->setInterval(1000);
    vtimerAutoRefresh->start();
    vtimerCountdown->start();
    connect(vtimerAutoRefresh, &QTimer::timeout, cclient, QOverload<>::of(&Client::fbaixaListas));
    connect(vtimerCountdown, &QTimer::timeout, this, QOverload<>::of(&MainClass::fupdateTimer));


    //After, we try to download the cover images
    cdownloader->setAvatar(cclient->fgetAvatar());
    cdownloader->fsetWorker();
    cdownloader->workAvatar(0);
    fdownloadCoverImages();

    //After, will look for anime episodes in your computer
    if(!tthreadDiretorios.isRunning()){
        cconfiguracoesUsuarioDiretorios->instance()->fgetThread(tthreadDiretorios);
        cconfiguracoesUsuarioDiretorios->instance()->moveToThread(&tthreadDiretorios);
        tthreadDiretorios.start();
    }
}

void MainClass::fconnectFail()
{
    //If the connection fails, we will try to read the anime list and connect again in a few seconds
    if(!vlistaSelecionada.isEmpty())
        finfoAnimeSelecionado(vposicaoGridAnimeSelecionado);
    //After, will look for anime episodes in your computer
    if(!tthreadDiretorios.isRunning()){
        cconfiguracoesUsuarioDiretorios->instance()->fgetThread(tthreadDiretorios);
        cconfiguracoesUsuarioDiretorios->instance()->moveToThread(&tthreadDiretorios);
        tthreadDiretorios.start();
    }

    QTimer::singleShot(10000, cclient, &Client::fbaixaListas);
}

void MainClass::fbotaoHome()
{
    emit sbotaoHome();
}

void MainClass::fbotaoConfig()
{
    emit sbotaoConfig();
}

void MainClass::fbotaoTorrent()
{
    emit sbotaoTorrent();
}

void MainClass::fsetauthCode(QVariant auth)
{
    cabaConfig->instance()->fsetAuthCode(auth);
}

void MainClass::fsetUsername(QVariant user)
{
    cabaConfig->instance()->fsetUsername(user);
}

void MainClass::fsetClient(QVariant client)
{
    cabaConfig->instance()->fsetService(client);
}

bool MainClass::fsetDirectory(QVariant dirPath)
{
    if(cabaConfig->instance()->fsetDirectory(dirPath))
        return true;
    return false;
}

bool MainClass::fremoveDirectory(QVariant dirPath)
{
    if(cabaConfig->instance()->fremoveDirectory(dirPath))
        return true;
    return false;
}

void MainClass::fsaveConfig()
{
    cabaConfig->instance()->fsaveSettings();
}

void MainClass::fsetDirConfig(QVariantList dirConfigs)
{
    qDebug() << dirConfigs;
    QSettings vset;
    cabaConfig->instance()->fsetHighQualityImages();
    qDebug() << vset.value("ConfigLowQuality").toString();;
}

QVariant MainClass::fgetDir()
{
    return cabaConfig->instance()->fgetDirectory();
}

void MainClass::finfoAnimeSelecionado(QVariant posicaoAnimeNaGrid)
{
    emit sdirImagensMedias(QVariant(cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensMedio));
    emit sdirImagensGrandes(QVariant(cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensGrandes));

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
        if(pix.load(cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensGrandes+
                    vlistaSelecionada[vindexAnimeSelecionado]->vid+".png","png"))
            emit simagemAnimeSelecionado(QVariant(cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensGrandes+
                                                  vlistaSelecionada[vindexAnimeSelecionado]->vid+".png"));
        else if(pix.load(cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensGrandes+
                         vlistaSelecionada[vindexAnimeSelecionado]->vid+".jpg", "jpg"))
            emit simagemAnimeSelecionado(QVariant(cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensGrandes+
                                                  vlistaSelecionada[vindexAnimeSelecionado]->vid+".jpg"));
        else
            emit simagemAnimeSelecionado(QVariant(cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensMedio+
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

QVariant MainClass::fretornaEpisodioAnimeEncontrado(QVariant posicao)
{
    if(vlistaSelecionada.size() > (12*(vpagina-1))+posicao.toInt()){
        QString episodePath = carquivos->fprocuraEpisodio(vlistaSelecionada[(12*(vpagina-1))+posicao.toInt()]);
        if(!episodePath.isEmpty())
            return QVariant("1");
        else if(vlistaSelecionada[(12*(vpagina-1))+posicao.toInt()]->vstatus.
                compare("Not Aired Yet", Qt::CaseInsensitive) == 0)
            return QVariant("-1");
    }
    return QVariant("0");
}

///TODO
QVariant MainClass::fretornaNomeUsuario()
{
    return QVariant("gutocbs");
}

QVariant MainClass::fretornaPathAvatar()
{
    return QVariant(cconfiguracoesDiretoriosPadrao->instance()->vimagemAvatar);
}

void MainClass::fupdateTimer()
{
    time = time.addSecs(-1);
    emit stimer(QVariant(time.toString("mm:ss")));
}

QVariant MainClass::fretornaNumeroAnos()
{
    return QVariant(QDate::currentDate().year()-1998);
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
    vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);
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

void MainClass::fselecionaTipoAnime()
{
    //Check if it is already the right type
    if(vtipoAtual != leitorlistaanimes::type::ANIME){
        vtipoAtual = leitorlistaanimes::type::ANIME;
        vlistaAtual = enumlistaToQString(lista::CURRENT);
        //Change the old list to the new
        vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);
        //Check if the list is empty for some reason
        if(!vlistaSelecionada.isEmpty()){
            vindexAnimeSelecionado = 0;
            vpagina = 1;
            finfoAnimeSelecionado(0);
            fdownloadCoverImages();
        }
    }
}

void MainClass::fselecionaTipoManga()
{
    //Check if it is already the right type
    if(vtipoAtual != leitorlistaanimes::type::MANGA){
        vtipoAtual = leitorlistaanimes::type::MANGA;
        vlistaAtual = enumlistaToQString(lista::CURRENT);
        //Change the old list to the new
        vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);
        //Check if the list is empty for some reason
        if(!vlistaSelecionada.isEmpty()){
            vindexAnimeSelecionado = 0;
            vpagina = 1;
            finfoAnimeSelecionado(0);
            fdownloadCoverImages();
        }
    }
}

void MainClass::fselecionaTipoNovel()
{
    //Check if it is already the right type
    if(vtipoAtual != leitorlistaanimes::type::NOVEL){
        vtipoAtual = leitorlistaanimes::type::NOVEL;
        vlistaAtual = enumlistaToQString(lista::CURRENT);
        //Change the old list to the new
        vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);
        //Check if the list is empty for some reason
        if(!vlistaSelecionada.isEmpty()){
            vindexAnimeSelecionado = 0;
            vpagina = 1;
            finfoAnimeSelecionado(0);
            fdownloadCoverImages();
        }
    }
}

void MainClass::fselecionaTipoSeason(QVariant data)
{
    vanoBuscaAnimes = data.toInt();
    vlistaAtual = QString::number(vanoBuscaAnimes);
    vtipoAtual = leitorlistaanimes::type::SEASON;
    vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);
    if(!vlistaSelecionada.isEmpty()){
        vindexAnimeSelecionado = 0;
        vpagina = 1;
        finfoAnimeSelecionado(0);
        fdownloadCoverImages();
    }
}

void MainClass::fselecionaListaCurrent()
{
    switch (vtipoAtual) {
    case leitorlistaanimes::type::SEASON:
        if(vlistaAtual.compare(enumlistaToQString(lista::WINTER)
                               +QString::number(vanoBuscaAnimes), Qt::CaseInsensitive) != 0){
            vlistaAtual = enumlistaToQString(lista::WINTER)+QString::number(vanoBuscaAnimes);
            //Change the old list to the new
            vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);
            //Check if the list is empty for some reason
            if(!vlistaSelecionada.isEmpty()){
                vindexAnimeSelecionado = 0;
                vpagina = 1;
                finfoAnimeSelecionado(0);
                fdownloadCoverImages();
            }
        }
        break;
    default:
        //Check if it is already the right list
        if(vlistaAtual.compare(enumlistaToQString(lista::CURRENT), Qt::CaseInsensitive) != 0){
            vlistaAtual = enumlistaToQString(lista::CURRENT);
            //Change the old list to the new
            vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);
            //Check if the list is empty for some reason
            if(!vlistaSelecionada.isEmpty()){
                vindexAnimeSelecionado = 0;
                vpagina = 1;
                finfoAnimeSelecionado(0);
                fdownloadCoverImages();
            }
        }
        break;
    }
}

void MainClass::fselecionaListaCompleted()
{
    switch (vtipoAtual) {
    case leitorlistaanimes::type::SEASON:
        if(vlistaAtual.compare(enumlistaToQString(lista::SPRING)
                               +QString::number(vanoBuscaAnimes), Qt::CaseInsensitive) != 0){
            vlistaAtual = enumlistaToQString(lista::SPRING)+QString::number(vanoBuscaAnimes);
            //Change the old list to the new
            vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);
            //Check if the list is empty for some reason
            if(!vlistaSelecionada.isEmpty()){
                vindexAnimeSelecionado = 0;
                vpagina = 1;
                finfoAnimeSelecionado(0);
                fdownloadCoverImages();
            }
        }
        break;
    default:
        //Check if it is already the right list
        if(vlistaAtual.compare(enumlistaToQString(lista::COMPLETED), Qt::CaseInsensitive) != 0){
            vlistaAtual = enumlistaToQString(lista::COMPLETED);
            //Change the old list to the new
            vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);
            //Check if the list is empty for some reason
            if(!vlistaSelecionada.isEmpty()){
                vindexAnimeSelecionado = 0;
                vpagina = 1;
                finfoAnimeSelecionado(0);
                fdownloadCoverImages();
            }
        }
        break;
    }
}

void MainClass::fselecionaListaPaused()
{
    switch (vtipoAtual) {
    case leitorlistaanimes::type::SEASON:
        if(vlistaAtual.compare(enumlistaToQString(lista::SUMMER)
                               +QString::number(vanoBuscaAnimes), Qt::CaseInsensitive) != 0){
            vlistaAtual = enumlistaToQString(lista::SUMMER)+QString::number(vanoBuscaAnimes);
            //Change the old list to the new
            vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);
            //Check if the list is empty for some reason
            if(!vlistaSelecionada.isEmpty()){
                vindexAnimeSelecionado = 0;
                vpagina = 1;
                finfoAnimeSelecionado(0);
                fdownloadCoverImages();
            }
        }
        break;
    default:
        //Check if it is already the right list
        if(vlistaAtual.compare(enumlistaToQString(lista::PAUSED), Qt::CaseInsensitive) != 0){
            vlistaAtual = enumlistaToQString(lista::PAUSED);
            //Change the old list to the new
            vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);
            //Check if the list is empty for some reason
            if(!vlistaSelecionada.isEmpty()){
                vindexAnimeSelecionado = 0;
                vpagina = 1;
                finfoAnimeSelecionado(0);
                fdownloadCoverImages();
            }
        }
        break;
    }
}

void MainClass::fselecionaListaDropped()
{
    switch (vtipoAtual) {
    case leitorlistaanimes::type::SEASON:
        if(vlistaAtual.compare(enumlistaToQString(lista::FALL)
                               +QString::number(vanoBuscaAnimes), Qt::CaseInsensitive) != 0){
            vlistaAtual = enumlistaToQString(lista::FALL)+QString::number(vanoBuscaAnimes);
            //Change the old list to the new
            vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);
            //Check if the list is empty for some reason
            if(!vlistaSelecionada.isEmpty()){
                vindexAnimeSelecionado = 0;
                vpagina = 1;
                finfoAnimeSelecionado(0);
                fdownloadCoverImages();
            }
        }
        break;
    default:
        //Check if it is already the right list
        if(vlistaAtual.compare(enumlistaToQString(lista::DROPPED), Qt::CaseInsensitive) != 0){
            vlistaAtual = enumlistaToQString(lista::DROPPED);
            //Change the old list to the new
            vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);
            //Check if the list is empty for some reason
            if(!vlistaSelecionada.isEmpty()){
                vindexAnimeSelecionado = 0;
                vpagina = 1;
                finfoAnimeSelecionado(0);
                fdownloadCoverImages();
            }
        }
        break;
    }
}

void MainClass::fselecionaListaPlanning()
{
    //Check if it is already the right list
    if(vlistaAtual.compare(enumlistaToQString(lista::PLANNING), Qt::CaseInsensitive) != 0){
        vlistaAtual = enumlistaToQString(lista::PLANNING);
        //Change the old list to the new
        vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);
        //Check if the list is empty for some reason
        if(!vlistaSelecionada.isEmpty()){
            vindexAnimeSelecionado = 0;
            vpagina = 1;
            finfoAnimeSelecionado(0);
            fdownloadCoverImages();
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
    QByteArray lcaminhoPasta = cconfiguracoesUsuarioDiretorios->instance()->fretornaDiretorioEspecifico(
                vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt()).toUtf8();
    if(!lcaminhoPasta.isEmpty())
        carquivos->fabreEpisodio(lcaminhoPasta);
}

void MainClass::fabreStream()
{
    QDesktopServices::openUrl(QUrl(vlistaSelecionada[vindexAnimeSelecionado]->vstreamCrunchyroll));
}

void MainClass::frefresh()
{
    cclient->fbaixaListas();
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

//Não funciona no caso de ter listas de anos
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
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("COMPLETED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Completed";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "COMPLETED");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PAUSED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "On Hold";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PAUSED");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("DROPPED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Dropped";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "DROPPED");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PLANNING") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Plan to Watch";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PLANNING");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        break;
    case leitorlistaanimes::type::MANGA:
        if(rnewList.compare("CURRENT") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Reading(Manga)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "CURRENT");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("COMPLETED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Completed(Manga)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "COMPLETED");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PAUSED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "On Hold(Manga)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PAUSED");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("DROPPED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Dropped(Manga)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "DROPPED");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PLANNING") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Plan to Read(Manga)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PLANNING");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        break;
    case leitorlistaanimes::type::NOVEL:
        if(rnewList.compare("CURRENT") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Reading(Novel)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "CURRENT");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("COMPLETED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Completed(Novel)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "COMPLETED");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PAUSED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "On Hold(Novel)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PAUSED");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("DROPPED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Dropped(Novel)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "DROPPED");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PLANNING") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Plan to Read(Novel)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PLANNING");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        break;
    case leitorlistaanimes::type::SEASON:
        if(rnewList.compare("CURRENT") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Watching";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "CURRENT");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, leitorlistaanimes::type::ANIME);
        }
        else if(rnewList.compare("COMPLETED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Completed";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "COMPLETED");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, leitorlistaanimes::type::ANIME);
        }
        else if(rnewList.compare("PAUSED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "On Hold";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PAUSED");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, leitorlistaanimes::type::ANIME);
        }
        else if(rnewList.compare("DROPPED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Dropped";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "DROPPED");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, leitorlistaanimes::type::ANIME);
        }
        else if(rnewList.compare("PLANNING") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Plan to Read";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PLANNING");
            cleitorListaAnimes->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, leitorlistaanimes::type::ANIME);
        }
        break;
    default:
        break;
    }

    //Update de GUI with the new, change, lists
    vlistaSelecionada = cleitorListaAnimes->instance()->sortLista(vordemLista, vlistaAtual, vtipoAtual);
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
