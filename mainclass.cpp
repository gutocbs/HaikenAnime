#include "mainclass.h"
#define enumlistaToQString(x) QString(vmetaEnumLista.valueToKey(x))
#define enumtipoToQString(x) QString(vmetaEnumTipo.valueToKey(x))

//MAIN CLASS é a classe que se comunica com QML

MainClass::MainClass(QObject *parent) : QObject(parent)
{
    vmetaEnumLista = QMetaEnum::fromType<lista>();
    vmetaEnumTipo = QMetaEnum::fromType<Database::type>();
    //Cria todas as variáveis
    cconfiguracoesDiretoriosPadrao = new confBase(nullptr);
    cdatabase = new Database(nullptr);
//    cdatabase->instance();
    cconfiguracoesUsuarioDiretorios = new confUsuario(nullptr);
    carquivos = new arquivos(this);
    cclient = new Client(this);
//    cdownloader = new Downloader(this);
    cabaConfig = new abaConfig(this);
    cabaTorrent = new abaTorrent(this);
    downloadQueue = new DownloadQueue(this);
    cclient->fselecionaClient(cabaConfig->instance()->fgetService());
    //cclient->frecebeAutorizacao(configurações->user, configurações->codigo)
    cclient->frecebeAutorizacao(cabaConfig->instance()->fgetUsername(), cabaConfig->instance()->fgetAuthCode());
    cclient->fbaixaListas();

    cconfiguracoesUsuarioDiretorios->instance()->frecebeConfigs(cabaConfig->instance()->fgetDirectory().toStringList());

    cdatabase->instance()->fcarregaIdNomeAno();
    mediaList = Enums::mediaList::CURRENT;
    vlistaSelecionada = cdatabase->instance()->returnAnimeList("CURRENT");


    cconfiguracoesDiretoriosPadrao->instance()->fcriaDiretoriosBase();
    cconfiguracoesUsuarioDiretorios->instance()->flePastasArquivos();

    vposicaoGridAnimeSelecionado = 0;
    vindexAnimeSelecionado = 0;
    vpagina = 1;
    vcontadorAssistindoEpisodio = 0;
    vrateLimitRequests = 0;
    vlistaAtual = enumlistaToQString(lista::CURRENT);
    vtipoAtual = Database::type::ANIME;
    vordemLista = "";
    vjanelaAtual = janela::MAIN;
    selectedPage = 1;


    cconfiguracoesUsuarioDiretorios->instance()->fgetThread(tthreadDiretorios);
    cconfiguracoesUsuarioDiretorios->instance()->moveToThread(&tthreadDiretorios);

    listUpdateCountdown = new QTimer(this);
    vtimerCountdown = new QTimer(this);
    vtimerChecaAssistindo = new QTimer(this);
    vtimerChecaAssistindo->setInterval(10000);
    vtimerChecaAssistindo->start();
    fconnections();
    fresetRequests();
    setObjects();
}

MainClass::~MainClass()
{
    if(tthreadDiretorios.isRunning()){
        tthreadDiretorios.requestInterruption();
        tthreadDiretorios.wait();
    }
    cconfiguracoesDiretoriosPadrao->deleteLater();
    cdatabase->deleteLater();
    cconfiguracoesUsuarioDiretorios->deleteLater();
}

void MainClass::fconnections()
{
    //Ao terminar de baixar a lista, começa a rodar o programa
    connect(cclient, &Client::sdownloadCompleted, this, &MainClass::ftryClientConnection);
    //Atualiza a tabela de torrents assim que terminar de ler o arquivo XML
    connect(cabaTorrent, &abaTorrent::sfimXML, this, &MainClass::storrentPronto);
    //Conecta o timer de checagem de animes com a função
    connect(vtimerChecaAssistindo, &QTimer::timeout, this, QOverload<>::of(&MainClass::fchecaAnimeAssistido));
    //Dá update no timer
    connect(vtimerCountdown, &QTimer::timeout, this, QOverload<>::of(&MainClass::fupdateTimer));

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
    if(tthreadDiretorios.isRunning())
        tthreadDiretorios.requestInterruption();
    //    cconfiguracoesUsuarioDiretorios->instance()->thread()->requestInterruption();
//    if(!vlistaSelecionada.isEmpty())
//        cdatabase->instance()->fdeletaListaAnimes();
    cdatabase->instance()->freadDatabaseFile();
    vlistaSelecionada = cdatabase->instance()->returnSortList(vordemLista, vlistaAtual, vtipoAtual);

    //We need to check if the list got empy before reading it
    if(vlistaSelecionada.size() < vindexAnimeSelecionado){
        vindexAnimeSelecionado = 0;
        vpagina = 1;
    }
    loadMediaList();
    getSelectedMediaData(0);
    emit sconnectGUI(true);

    //The auto-update timer starts
    time = QTime::fromString("10","m");
    vtimerCountdown->setInterval(1000);
    vtimerCountdown->start();


    //After, we try to download the cover images
//    cdownloader->setAvatar(cclient->fgetAvatar());
//    cdownloader->fsetWorker();
//    cdownloader->workAvatar(0);
    downloadQueue->downloadMedia();
    //TODO - COLOCAR SEMPRE QUE PRECISAR MUDAR DE PÁGINA OU BAIXAR UMA NOVA IMAGEM
    //mediaDownloader->insertDownloadQueue(vlistaAtual, mediaType, mediaList,vlistaSelecionada.size());
//    mediaDownloader->downloadCoverImages();

    //After, will look for anime episodes in your computer
    if(!tthreadDiretorios.isRunning()){
        tthreadDiretorios.start();
    }
}

void MainClass::connectSuccess()
{
    //block buttons
    emit sconnectGUI(false);
    loadMediaList();

    //Read info from the selected anime
    finfoAnimeSelecionado(0);
    emit sconnectGUI(true);
    setUpdateTimer();
    setDownloads();
    //TODO - Ler comentário a baixo sobre ser a primeira conexão
    //Setar uma variável pra ver se é a primeira vez que conecta. Se for, pode dar o setUpdate.
    //Caso não, nào deve dar o setUpdate
    clientManager->setUpdate();
    //TODO - Startar busca por animes
}

void MainClass::loadMediaList()
{
    //TODO - If the list is not empty, we have to empty it first.
    MediaLoader::loadMediaFromFile();
    //We need to check if the list got empy before reading it
    if(mediaListManager->getInstance()->size(mediaList) < selectedMediaIndex){
        selectedMediaIndex = 0;
        selectedPage = 1;
    }
}

void MainClass::setUpdateTimer()
{
    listUpdateTimer = QTime::fromString("10","m");
    listUpdateCountdown->setInterval(1000);
    listUpdateCountdown->start();
}

void MainClass::setDownloads()
{
    //TODO - Download de avatar
//    downloadQueue->downloadAvatar();
    downloadQueue->downloadMedia();
}

int MainClass::getPageIndexRange()
{
    int mediaNumberPerPage{12};
    //-1 since the page starts at 1, not 0
    return mediaNumberPerPage*(selectedPage-1);
}

void MainClass::setObjects()
{
    downloadQueue = new DownloadQueue(this);
    mediaListManager = new AnimeListManager(this);
    clientManager = new ClientManager(this);
    mediaPlayer = new MediaPlayer(this);
    mediaSearchManager = new MediaSearchManager(this);
    mediaSearchManager->setMediaListManager(mediaListManager);
    mediaManager = new AnimeManager(this);
    mediaManager->getInstance()->setMediaListManager(mediaListManager);
    mediaManager->getInstance()->setMediaSearchManager(mediaSearchManager);
    loadMediaList();
    getSelectedMediaData(0);
}

void MainClass::fconnectFail()
{
    cclient->frecebeAutorizacao(cabaConfig->instance()->fgetUsername(), cabaConfig->instance()->fgetAuthCode());
    //If the connection fails, we will try to read the anime list and connect again in a few seconds
    if(!vlistaSelecionada.isEmpty())
        finfoAnimeSelecionado(vposicaoGridAnimeSelecionado);
    //After, will look for anime episodes in your computer
    if(!tthreadDiretorios.isRunning()){
        tthreadDiretorios.start();
    }

    QTimer::singleShot(10000, cclient, &Client::fbaixaListas);
}

void MainClass::fbotaoHome()
{
    vjanelaAtual = janela::MAIN;
    emit sbotaoHome();
}

void MainClass::fbotaoConfig()
{
    vjanelaAtual = janela::CONFIG;
    emit sbotaoConfig();
}

void MainClass::fbotaoTorrent()
{
    vjanelaAtual = janela::TORRENT;
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

void MainClass::fsetTorrentList()
{
    cabaTorrent->fgetTorrentList();
}

QVariant MainClass::fgetTorrentList(QVariant ordem)
{
    return QVariant(cabaTorrent->fgetTorrentInfo(ordem.toString()));
}

void MainClass::fchangeTorrentState(QVariant posicaoTabela, QVariant checkState)
{
    cabaTorrent->fchangeTorrentState(posicaoTabela.toInt(), checkState.toBool());
}

void MainClass::fsearchAnimeFromTorrent(QVariant posicaoTabela)
{
    QVariant infoAnime = cabaTorrent->fgetSingleTorrentInfo(posicaoTabela.toInt());
    if(!infoAnime.toInt())
        fbotaoBusca(infoAnime.toStringList().at(1));
}

QVariant MainClass::fopenTorrentLink(QVariant posicaoTabela)
{
    QVariant infoAnime = cabaTorrent->fgetSingleTorrentInfo(posicaoTabela.toInt());
    if(!infoAnime.toInt())
        return infoAnime.toStringList().at(8);
    return "";
}



void MainClass::fbotaoBusca(QVariant search)
{
    QString searchText = search.toString();
    //Caso seja a janela de mangas e animes
    if(vjanelaAtual == janela::MAIN){
        if(!searchText.isEmpty()){
            qDebug() << "Searching " << searchText;
            vlistaSelecionada = cdatabase->instance()->returnSearchList(searchText.simplified(), vtipoAtual);
            if(!vlistaSelecionada.isEmpty()){
                vlistaAtual = enumlistaToQString(lista::SEARCH);
                mediaDownloader->downloadCoverImages();
                vindexAnimeSelecionado = 0;
                vpagina = 1;
    //            ui->NumPagina->setText("Busca  - " + QString::number(vlistaSelecionada.size()) +
    //                                   " animes in the list - Page "+QString::number(vpagina)+"/"+
    //                                   QString::number(((vlistaSelecionada.size()-1)/12)+1));
                finfoAnimeSelecionado(0);
            }
            else{
                qDebug() << searchText << " not found!";
                vlistaSelecionada = cdatabase->instance()->returnSortList(vordemLista, vlistaAtual, vtipoAtual);
            }
        }
    }
    //Caso seja a janela de torrent
    else if(vjanelaAtual == janela::TORRENT){
        if(!searchText.isEmpty())
            cabaTorrent->fgetSpecificTorrentList(searchText);
    }
}

void MainClass::fbotaoDownloadTorrents()
{
    cabaTorrent->fdownloadAnimes();
}

void MainClass::fclientUpdate()
{
    if(vlistaAcoes.isEmpty())
        return;
    QString nome;
    foreach (QStringList key, vlistaAcoes.keys()) {
        nome = cdatabase->instance()->fbuscaIDRetornaTitulo(key.at(1));
        if(vrateLimitRequests == 90)
            return;
        else if(key.at(0) == "nota"){
            if(cclient->fmudaNota(key.at(1).toInt(), vlistaAcoes[key].toInt())){
                qDebug() << QString("Change: Score - " + nome + " - " + vlistaAcoes[key]
                                     + " - " + QDateTime::currentDateTime().toString());
                vlistaAcoes.remove(key);
            }
            else
                qDebug() << "Couldn't reach the client server. Trying again in 10 seconds.";
        }
        else if(key.at(0) == "progresso"){
            if(cclient->fmudaProgresso(key.at(1).toInt(), vlistaAcoes[key].toInt())){
                qDebug() << QString("Change: Progress - " + nome +
                                    " - " + vlistaAcoes[key] + " - " + QDateTime::currentDateTime().toString());
                vlistaAcoes.remove(key);
            }
            else
                qDebug() << "Couldn't reach the client server. Trying again in 10 seconds.";
        }
//        else if(key.at(0) == "lista"){
//            if(cclient->fmudaLista(key.at(1).toInt(), vlistaAcoes[key])){
//                qDebug() << QString("Change: List - " + nome +
//                                    " - " + vlistaAcoes[key] + " - " + QDateTime::currentDateTime().toString());
//                vlistaAcoes.remove(key);
//            }
//            else
//                qDebug() << "Couldn't reach the client server. Trying again in 10 seconds.";
//        }
        else if(key.at(0) == "remove"){
            if(cclient->fexcluiAnime(key.at(1).toInt())){
                qDebug() << QString("Change: Remove - " + nome +
                                    " - " + vlistaAcoes[key] + " - " + QDateTime::currentDateTime().toString());
                vlistaAcoes.remove(key);
            }
            else
                qDebug() << "Couldn't reach the client server. Trying again in 10 seconds.";
        }
        vrateLimitRequests++;
    }
}

void MainClass::finfoAnimeSelecionado(QVariant posicaoAnimeNaGrid)
{
//    emit sdirImagensPequenas(QVariant(cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensPequenas));
    emit sdirImagensMedias(QVariant(cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensMedio));
    emit sdirImagensGrandes(QVariant(cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensGrandes));

    bool ok;
    vposicaoGridAnimeSelecionado = posicaoAnimeNaGrid.toInt(&ok);
    if(ok)
        vindexAnimeSelecionado = 12*(vpagina-1)+vposicaoGridAnimeSelecionado;

    if(vlistaSelecionada.size() > vindexAnimeSelecionado && cdatabase->instance()->fchecaDatabaseReady()){
        emit snomeAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vnome));
        emit snomeAlternativoAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vnomeIngles));
        emit ssinopseAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vsinopse));
        emit sstatusAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vstatus));
        emit sseasonAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vseason));
        emit smediaGloballAnimeSelecionado(QVariant(QString::number(vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaSite.toFloat()/10, 'f', 2)));
        emit smediaPessoalAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaPessoal));
        emit sreleaseAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vdataEpisodio));
        if(vlistaSelecionada[vindexAnimeSelecionado]->vnumProximoEpisodioLancado == "" &&
                vlistaSelecionada[vindexAnimeSelecionado]->vstatus != "Finished Airing")
            emit sepisodiosLancadosAnimeSelecionado(QVariant("-"));
        else if(vlistaSelecionada[vindexAnimeSelecionado]->vnumProximoEpisodioLancado.toInt() == 0)
            emit sepisodiosLancadosAnimeSelecionado(QVariant("All Episodes"));
        else{
            if(vlistaSelecionada[vindexAnimeSelecionado]->vnumProximoEpisodioLancado.toInt() - 1 == 1)
                emit sepisodiosLancadosAnimeSelecionado(QVariant(
                    QString::number(vlistaSelecionada[vindexAnimeSelecionado]->vnumProximoEpisodioLancado.toInt() - 1) + " Episode"));
            else if(vlistaSelecionada[vindexAnimeSelecionado]->vnumProximoEpisodioLancado.toInt() - 1 == 0)
                emit sepisodiosLancadosAnimeSelecionado(QVariant("No Episodes"));
            else
                emit sepisodiosLancadosAnimeSelecionado(QVariant(
                    QString::number(vlistaSelecionada[vindexAnimeSelecionado]->vnumProximoEpisodioLancado.toInt() - 1) + " Episodes"));
        }
        emit sepisodiosAssistidosAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosAssistidos));
        emit sepisodiosTotaisAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosTotais));
        emit stipoAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vformato));
        QString episodePath = carquivos->fprocuraEpisodio(vlistaSelecionada[vindexAnimeSelecionado]);
        if(!episodePath.isEmpty())
            emit sproximoEpisodioAnimeSelecionado(true);
        else
            emit sproximoEpisodioAnimeSelecionado(false);
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
        emit sidAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vid));
    }
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

QVariant MainClass::fretornaNomeUsuario()
{
    return QVariant(cabaConfig->instance()->fgetUsername());
}

QVariant MainClass::fretornaPathAvatar()
{
    return QVariant(cconfiguracoesDiretoriosPadrao->instance()->vimagemAvatar);
}

void MainClass::fupdateTimer()
{
    if(time == QTime(0,0,0)){
        time = QTime::fromString("10","m");
        cclient->fbaixaListas();
    }
    else
        time = time.addSecs(-1);

    emit stimer(QVariant(time.toString("mm:ss")));
}

QVariant MainClass::fretornaNumeroAnos()
{
    return QVariant(QDate::currentDate().year()-1998);
}

void MainClass::getMediaList(QVariant order, QVariant year, bool changeOrder)
{
    if(changeOrder)
        mediaListManager->getInstance()->setListOrder(order);
    mediaListManager->getInstance()->sortList(mediaList, year);
    //Check if the list is empty for some reason
    if(mediaListManager->getInstance()->size(mediaList) < selectedMediaIndex){
        selectedMediaIndex = 0;
        selectedPage = 1;
    }
    getSelectedMediaData(selectedMediaIndex);
}

void MainClass::fabreSite(QVariant data)
{
    bool ok;
    data.toInt(&ok);
    if(ok){
        QDesktopServices::openUrl(QUrl(vlistaSelecionada[vindexAnimeSelecionado]->vdatabaseSite));
    }
}

void MainClass::fselecionaTipoAnime()
{
    //Check if it is already the right type
    if(vtipoAtual != Database::type::ANIME){
        vtipoAtual = Database::type::ANIME;
        vlistaAtual = enumlistaToQString(lista::CURRENT);
        //Change the old list to the new
        vlistaSelecionada = cdatabase->instance()->returnSortList(vordemLista, vlistaAtual, vtipoAtual);
        //Check if the list is empty for some reason
        if(!vlistaSelecionada.isEmpty()){
            vindexAnimeSelecionado = 0;
            vpagina = 1;
            finfoAnimeSelecionado(0);
//            mediaDownloader->downloadCoverImages();
            downloadQueue->downloadMedia();
        }
    }
}

void MainClass::selectTypeAnime()
{
    if(mediaType != Enums::mediaType::ANIME){
        mediaType = Enums::mediaType::ANIME;
        mediaList = Enums::mediaList::CURRENT;
        mediaListManager = new AnimeListManager(this);
        mediaManager = new AnimeManager(this);
        mediaSearchManager->setMediaListManager(mediaListManager);
        mediaManager->getInstance()->setMediaListManager(mediaListManager);
        mediaManager->getInstance()->setMediaSearchManager(mediaSearchManager);
        getMediaList();
    }
}

void MainClass::selectTypeManga()
{
    if(mediaType != Enums::mediaType::MANGA){
        mediaType = Enums::mediaType::MANGA;
        mediaList = Enums::mediaList::CURRENT;
        mediaListManager = new MangaListManager(this);
        mediaManager = new MangaManager(this);
        mediaSearchManager->setMediaListManager(mediaListManager);
        mediaManager->getInstance()->setMediaListManager(mediaListManager);
        mediaManager->getInstance()->setMediaSearchManager(mediaSearchManager);
        getMediaList();
    }
}

void MainClass::selectTypeNovel()
{
    if(mediaType != Enums::mediaType::NOVEL){
        mediaType = Enums::mediaType::NOVEL;
        mediaList = Enums::mediaList::CURRENT;
        mediaListManager = new NovelListManager(this);
        mediaManager = new NovelManager(this);
        mediaSearchManager->setMediaListManager(mediaListManager);
        mediaManager->getInstance()->setMediaListManager(mediaListManager);
        mediaManager->getInstance()->setMediaSearchManager(mediaSearchManager);
        getMediaList();
    }
}

//TODO - Fazer mudança do botão pra lista de cada season
void MainClass::selectListCurrent()
{
    if(mediaList != Enums::mediaList::CURRENT){
        mediaList = Enums::mediaList::CURRENT;
        getMediaList();
    }
}

void MainClass::selectListCompleted()
{
    if(mediaList != Enums::mediaList::COMPLETED){
        mediaList = Enums::mediaList::COMPLETED;
        getMediaList();
    }
}

void MainClass::selectListPaused()
{
    if(mediaList != Enums::mediaList::PAUSED){
        mediaList = Enums::mediaList::PAUSED;
        getMediaList();
    }
}

void MainClass::selectListDropped()
{
    if(mediaList != Enums::mediaList::DROPPED){
        mediaList = Enums::mediaList::DROPPED;
        getMediaList();
    }
}

void MainClass::selectListPlanning()
{
    if(mediaList != Enums::mediaList::PLANNING){
        mediaList = Enums::mediaList::PLANNING;
        getMediaList();
    }
}

void MainClass::playNextEpisode()
{
//    MediaManager::playMediaNextEpisode(mediaListManager->getInstance()->getMediaByIndex(mediaList,selectedMediaIndex));
}

void MainClass::getMediaListPage()
{
    int mediaListIndex;
    for(int i = 0; i < 12; i++)
    {
        mediaListIndex = (getPageIndexRange())+i;
        if(mediaListManager->getInstance()->size(mediaList) > mediaListIndex)
            emitSignalIdMedia(i);
        else
            emitSignalIdMedia(i, true);
    }
}

void MainClass::emitSignalIdMedia(int listMediaIndex, bool nullSignal)
{
    QVariant signalId;
    if(nullSignal)
        signalId = QVariant("null");
    else
        signalId = QVariant(mediaListManager->getInstance()->getMediaByIndex(mediaList,listMediaIndex)->id);
    if(listMediaIndex == 0)
        emit signalIdMediaGrid0(signalId);
    if(listMediaIndex == 1)
        emit signalIdMediaGrid1(signalId);
    if(listMediaIndex == 2)
        emit signalIdMediaGrid2(signalId);
    if(listMediaIndex == 3)
        emit signalIdMediaGrid3(signalId);
    if(listMediaIndex == 4)
        emit signalIdMediaGrid4(signalId);
    if(listMediaIndex == 5)
        emit signalIdMediaGrid5(signalId);
    if(listMediaIndex == 6)
        emit signalIdMediaGrid6(signalId);
    if(listMediaIndex == 7)
        emit signalIdMediaGrid7(signalId);
    if(listMediaIndex == 8)
        emit signalIdMediaGrid8(signalId);
    if(listMediaIndex == 9)
        emit signalIdMediaGrid9(signalId);
    if(listMediaIndex == 10)
        emit signalIdMediaGrid10(signalId);
    if(listMediaIndex == 11)
        emit signalIdMediaGrid11(signalId);

}

void MainClass::getSelectedMediaData(QVariant selectedMediaGridIndex)
{
//    emit sdirImagensPequenas(QVariant(cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensPequenas));
    emit sdirImagensMedias(QVariant(cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensMedio));
    emit sdirImagensGrandes(QVariant(cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensGrandes));

    bool ok;
    this->selectedMediaGridIndex = selectedMediaGridIndex.toInt(&ok);
    if(ok)
        selectedMediaIndex = getPageIndexRange()+this->selectedMediaGridIndex;

    if(mediaListManager->getInstance()->size(mediaList) > selectedMediaIndex){
        QJsonObject selectedMediaObject = MediaUtil::getMediaAsJsonObject(mediaListManager->getInstance()->getMediaByIndex(mediaList, selectedMediaIndex));
        emit signalSelectedMediaCover(QVariant(selectedMediaObject["coverImagePath"]));
        emit signalSelectedMedia(QVariant(selectedMediaObject));
    }
    getMediaListPage();
}

QVariant MainClass::getMediaJsonObjectByGridIndex(QVariant gridIndex)
{
    if(mediaListManager->getInstance()->size(mediaList) > (getPageIndexRange())+gridIndex.toInt())
        return QVariant(MediaUtil::getMediaAsJsonObject(mediaListManager->getInstance()->getMediaByIndex(mediaList, getPageIndexRange()+gridIndex.toInt())));
    return QVariant("");
}

QVariant MainClass::getUsername()
{
    return QVariant(cabaConfig->instance()->fgetUsername());
}

QVariant MainClass::getUserAvatar()
{
    return QVariant(cconfiguracoesDiretoriosPadrao->instance()->vimagemAvatar);
}

void MainClass::openMediaWebpage(QVariant data)
{
    bool ok;
    data.toInt(&ok);
    if(ok){
        QDesktopServices::openUrl(QUrl(mediaListManager->getInstance()->getMediaByIndex(mediaList,selectedMediaIndex)->siteURL));
    }
}

void MainClass::selectListSeason(QVariant data)
{
    if(mediaList != Enums::mediaList::CURRENT){
        mediaList = Enums::mediaList::YEAR;
        getMediaList(Enums::mediaOrder::StartDate, data.toInt());
    }
}

void MainClass::openMediaFolder()
{
    FileManager::openFileOrFolder(mediaListManager->getInstance()->getMediaByIndex(mediaList,selectedMediaIndex));
}

void MainClass::refreshMediaList()
{
    clientManager->downloadMediaList();
}

void MainClass::getCurrentMediaPlaying()
{
    QPointer<CurrentMediaPlaying> currentMediaPlaying = mediaPlayer->getCurrentMediaPlaying();
    if(!currentMediaPlaying.isNull()){
        emit currentMediaPlayer(QVariant(currentMediaPlaying->mediaId),
                                QVariant(currentMediaPlaying->mediaName),
                                QVariant(currentMediaPlaying->mediaEpisode));

        if(currentMediaPlayingCounter == 20){
            currentMediaPlayingCounter++;
            if(currentMediaPlaying->mediaEpisode > mediaSearchManager->getMediaEpisodeFromId(currentMediaPlaying->mediaId))
                clientManager->addToUpdateQueue(ClientEnums::PROGRESS, currentMediaPlaying->mediaId,currentMediaPlaying->mediaEpisode);
        }
        else if(currentMediaPlayingCounter < 20)
            currentMediaPlayingCounter++;
    }
    else{
        currentMediaPlayingCounter = 0;
        emit currentMediaPlayer(QVariant(""), QVariant(""), QVariant(""));
    }
    QTimer::singleShot(10, this, &MainClass::getCurrentMediaPlaying);
}

void MainClass::setMediaProgress(int mediaId, int mediaProgress)
{
    mediaManager->getInstance()->updateProgress(mediaId, mediaProgress);
    clientManager->addToUpdateQueue(ClientEnums::PROGRESS, mediaId, mediaProgress);
    getMediaList();
}

void MainClass::buttonSetMediaProgress(QVariant data)
{
    QPointer<Media> selectedMedia = mediaListManager->getInstance()->getMediaByIndex(mediaList,selectedMediaIndex);
    int progress{1};
    //TODO - Fazer enums pra isso
    if(data.toString().compare("diminui") == 0)
        progress = -1;
    if(selectedMedia->progress+progress >= 0 && selectedMedia->progress+progress < MediaUtil::getTotalEpisodes(selectedMedia))
        setMediaProgress(selectedMedia->id, selectedMedia->progress+progress);
}

void MainClass::buttonSetMediaScore(QVariant data)
{
    QPointer<Media> selectedMedia = mediaListManager->getInstance()->getMediaByIndex(mediaList,selectedMediaIndex);
    int score{selectedMedia->personalScore.toInt()};
    int maxScore{10};
    //TODO - Fazer enums pra isso
    if(data.toString().compare("diminui") == 0)
        score--;
    if(selectedMedia->personalScore+score >= 0 && selectedMedia->personalScore+score < maxScore){
        mediaManager->getInstance()->updateScore(selectedMedia->id, QString::number(score));
        clientManager->addToUpdateQueue(ClientEnums::SCORE, selectedMedia->id, score);
        getMediaList();
    }
}

void MainClass::setMediaCustomName(QVariant data)
{
    if(!data.toString().isEmpty())
        mediaManager->getInstance()->insertCustomName(mediaListManager->getInstance()->getMediaByIndex(mediaList,selectedMediaIndex)->id, data.toStringList());
}

void MainClass::removeMediaFromList()
{
    QPointer<Media> selectedMedia = mediaListManager->getInstance()->getMediaByIndex(mediaList,selectedMediaIndex);
    clientManager->addToUpdateQueue(ClientEnums::LIST, selectedMedia->id, Enums::mediaList::NOLIST);
    mediaManager->deleteFromList(selectedMedia->id);
    getMediaList();
}

void MainClass::buttonNextPage()
{
    int mediaNumberPerPage{12};
    if(mediaListManager->getInstance()->size(mediaList) > mediaNumberPerPage+(getPageIndexRange())){
        selectedPage++;
        getMediaListPage();
    }
}

void MainClass::buttonLastPage()
{
    if(selectedPage > 1){
        selectedPage--;
        getMediaListPage();
    }
}

void MainClass::setMediaList(QVariant data)
{
    Enums::mediaList newMediaList = Enums::QStringToMediaList(data.toString());
    //Check if the user is trying to change the entry from list A to list A
    if(mediaList == newMediaList)
        return;
    mediaManager->updateMediaList(mediaListManager->getInstance()->getMediaByIndex(mediaList,selectedMediaIndex)->id, newMediaList);
    clientManager->addToUpdateQueue(ClientEnums::updateType::LIST, mediaListManager->getInstance()->getMediaByIndex(mediaList,selectedMediaIndex)->id, newMediaList);
    getMediaList();
}

void MainClass::buttonMenuMedia()
{
    selectedMenu = menu::MEDIA;
    emit signalMenuMedia();
}

void MainClass::buttonMenuConfigurations()
{
    selectedMenu = menu::CONFIGURATION;
    emit signalMenuConfiguration();
}

void MainClass::buttonMenuTorrent()
{
    selectedMenu = menu::TORRENTS;
    emit signalMenuTorrents();
}

void MainClass::buttonSearch(QVariant data)
{
    QString searchText = data.toString();
    if(selectedMenu == menu::MEDIA && !searchText.isEmpty()){
        QVector<Media*> searchList = mediaSearchManager->searchMedia(data.toString());
        if(!searchList.isEmpty()){
            mediaList = Enums::mediaList::SEARCH;
            getMediaList();
        }
    }
    //Caso seja a janela de torrent
    else if(vjanelaAtual == janela::TORRENT){
        if(!searchText.isEmpty())
            cabaTorrent->fgetSpecificTorrentList(searchText);
    }
}

void MainClass::fselecionaTipoManga()
{
    //Check if it is already the right type
    if(vtipoAtual != Database::type::MANGA){
        vtipoAtual = Database::type::MANGA;
        vlistaAtual = enumlistaToQString(lista::CURRENT);
        //Change the old list to the new
        vlistaSelecionada = cdatabase->instance()->returnSortList(vordemLista, vlistaAtual, vtipoAtual);
        //Check if the list is empty for some reason
        if(!vlistaSelecionada.isEmpty()){
            vindexAnimeSelecionado = 0;
            vpagina = 1;
            finfoAnimeSelecionado(0);
//            mediaDownloader->downloadCoverImages();
            downloadQueue->downloadMedia();
        }
    }
}

void MainClass::fselecionaTipoNovel()
{
    //Check if it is already the right type
    if(vtipoAtual != Database::type::NOVEL){
        vtipoAtual = Database::type::NOVEL;
        vlistaAtual = enumlistaToQString(lista::CURRENT);
        //Change the old list to the new
        vlistaSelecionada = cdatabase->instance()->returnSortList(vordemLista, vlistaAtual, vtipoAtual);
        //Check if the list is empty for some reason
        if(!vlistaSelecionada.isEmpty()){
            vindexAnimeSelecionado = 0;
            vpagina = 1;
            finfoAnimeSelecionado(0);
//            mediaDownloader->downloadCoverImages();
            downloadQueue->downloadMedia();
        }
    }
}

void MainClass::fselecionaTipoSeason(QVariant data)
{
    vanoBuscaAnimes = data.toInt();
    vlistaAtual = QString::number(vanoBuscaAnimes);
    vtipoAtual = Database::type::SEASON;
    vlistaSelecionada = cdatabase->instance()->returnSortList(vordemLista, vlistaAtual, vtipoAtual);
    if(!vlistaSelecionada.isEmpty()){
        vindexAnimeSelecionado = 0;
        vpagina = 1;
        finfoAnimeSelecionado(0);
//        mediaDownloader->downloadCoverImages();
        downloadQueue->downloadMedia();
    }
}

///Se a função de todas as listas forem iguais, posso fazer uma função e passar o lista::Current como referência
//void MainClass::fselecionaListaCurrent()
//{
//    switch (vtipoAtual) {
//    case Database::type::SEASON:
//        if(vlistaAtual.compare(enumlistaToQString(lista::WINTER)
//                               +QString::number(vanoBuscaAnimes), Qt::CaseInsensitive) != 0){
//            vlistaAtual = enumlistaToQString(lista::WINTER)+QString::number(vanoBuscaAnimes);
//            //Change the old list to the new
//            vlistaSelecionada = cdatabase->instance()->returnSortList(vordemLista, vlistaAtual, vtipoAtual);
//            //Check if the list is empty for some reason
//            if(!vlistaSelecionada.isEmpty()){
//                vindexAnimeSelecionado = 0;
//                vpagina = 1;
//                finfoAnimeSelecionado(0);
//                mediaDownloader->downloadCoverImages();
//                downloadQueue->downloadMedia();
//            }
//        }
//        break;
//    default:
//        //Check if it is already the right list
//        if(vlistaAtual.compare(enumlistaToQString(lista::CURRENT), Qt::CaseInsensitive) != 0){
//            vlistaAtual = enumlistaToQString(lista::CURRENT);
//            //Change the old list to the new
//            vlistaSelecionada = cdatabase->instance()->returnSortList(vordemLista, vlistaAtual, vtipoAtual);
//            //Check if the list is empty for some reason
//            if(!vlistaSelecionada.isEmpty()){
//                vindexAnimeSelecionado = 0;
//                vpagina = 1;
//                finfoAnimeSelecionado(0);
//                mediaDownloader->downloadCoverImages();
//                downloadQueue->downloadMedia();
//            }
//        }
//        break;
//    }
//}



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
    QDesktopServices::openUrl(QUrl(vlistaSelecionada[vindexAnimeSelecionado]->vstream));
}

void MainClass::frefresh()
{
    cclient->fbaixaListas();
}

void MainClass::fchecaAnimeAssistido()
{
    //Crio uma variável da lib Robot para monitorar as janelas abertas
    QScopedPointer<Robot::Window> llistaJanelas(new Robot::Window);
    QString nomejanela;
    //Pego o nome de todos os players selecionados nas configurações para reconhecimento
    QStringList lplayers = cabaConfig->instance()->fgetPlayers();
    //Pego o nome de todas as janelas abertas
    auto vlistaJanelas = llistaJanelas->GetList();
    //Começa a comparar os nomes de cada janela com os da lista de players
    foreach (const auto& janela, vlistaJanelas){
        if(!QString::fromStdString(janela.GetTitle()).isEmpty()){
            nomejanela = QString::fromStdString(janela.GetTitle());
            foreach(QString player, lplayers){
                if(player.isEmpty())
                    break;
                //Algumas janelas não especificam o player no nome, então temos que incluí-los de acordo com o padrão
                //Do titulo da janela e a extensão do arquivo
                if(nomejanela.contains("online in high quality - ") && player.compare("kissanime", Qt::CaseInsensitive) == 0)
                    nomejanela.replace("online in high quality", "kissanime"); //Sim, é uma gambiarra.
                else if(nomejanela.contains(".mkv", Qt::CaseInsensitive) || nomejanela.contains(".mp4", Qt::CaseInsensitive)){
                        if(player.compare("Media Player Classic", Qt::CaseInsensitive) == 0)
                            nomejanela.append(" - Media Player Classic");
                        else if(player.compare("WebTorrent Desktop", Qt::CaseInsensitive) == 0)
                            nomejanela.append(" - WebTorrent Desktop");
                        else if(player.compare("Baka MPlayer", Qt::CaseInsensitive) == 0)
                            nomejanela.append(" - Baka MPlayer");
                }
                //Caso a janela do player tenha sido encontrada, é hora de descobrir qual anime está sendo visto.
                if(nomejanela.contains(player, Qt::CaseInsensitive)){
                    QPointer<playerRecognition> c(new playerRecognition);
                    c->fchecaStream(player, nomejanela);
                    QString nomeAnime = c->fretornaAnime();
                    QString episodio = c->fretornaEpisodio();
                    QString vidAnime = cdatabase->instance()->fbuscaNomeRetornaID(nomeAnime);
                    if(!vidAnime.isEmpty()){
                        emit sanimeReconhecidoID(QVariant(vidAnime), QVariant(nomeAnime), QVariant(episodio));
                        //Vai checar a cada 10 segundos, então tem que assistir dois minutos de anime pra contar. Assim, atualiza os curtas ao mesmo
                        //tempo que não atualiza se você mal assistir
                        if(vcontadorAssistindoEpisodio == 20){
                            vcontadorAssistindoEpisodio++;
                            fAumentaProgressoID(vidAnime, episodio);
                        }
                        else if(vcontadorAssistindoEpisodio == 21)
                            return;
                        else
                            vcontadorAssistindoEpisodio++;
                        return;
                    }
                }
            }
        }
    }
    vcontadorAssistindoEpisodio = 0;
    emit sanimeReconhecidoID(QVariant(""), QVariant(""), QVariant(""));
}

void MainClass::fAumentaProgressoID(const QString &ridAnime, const QString &episodioAnime)
{
    //A função vai rodar tudo em uma lista temporária e dar refresh na lista atual, pra mudar o progresso
    //Para isso, é preciso ter a posição e a lista do anime salvas
    QString llistaAtual = vlistaSelecionada[vindexAnimeSelecionado]->vlista;
    int lposicao = cdatabase->instance()->fbuscaIDRetornaPosicao(ridAnime);
    QString llistaAnimeAssistindo = cdatabase->instance()->fbuscaIDRetornaLista(ridAnime);
    QVector<anime*> listaTemp;
    //Checa se o ID do anime existe em alguma lista
    if(llistaAnimeAssistindo.isEmpty())
        return;
    //Olha todas as listas, exceto completed. Não tem sentido aumentar o progresso de algo já completo.
    if(llistaAnimeAssistindo.compare("Watching",Qt::CaseInsensitive) == 0){
        listaTemp = cdatabase->instance()->returnAnimeList("CURRENT");
    }
    else if(llistaAnimeAssistindo.compare("On Hold",Qt::CaseInsensitive) == 0){
        listaTemp = cdatabase->instance()->returnAnimeList("PAUSED");
    }
    else if(llistaAnimeAssistindo.compare("Dropped",Qt::CaseInsensitive) == 0){
        listaTemp = cdatabase->instance()->returnAnimeList("DROPPED");
    }
    else if(llistaAnimeAssistindo.compare("Plan to Watch",Qt::CaseInsensitive) == 0){
        listaTemp = cdatabase->instance()->returnAnimeList("PLANNING");
    }
    else{
        bool ok;
        //Checa se a lista é um número válido, pro caso das listas anuais
        llistaAnimeAssistindo.toInt(&ok);
        if(ok){
            listaTemp = cdatabase->instance()->returnAnimeYearlyList(llistaAnimeAssistindo.toInt());
        }
    }
    //Checa se a lista obtida está vazia
    if(listaTemp.isEmpty())
        return;
    if(lposicao != -1 && listaTemp[lposicao]->vnumEpisodiosAssistidos.toInt() < episodioAnime.toInt()){
        for(int i = 0; i < episodioAnime.toInt()-listaTemp[lposicao]->vnumEpisodiosAssistidos.toInt(); i++){
            int lepisodiosTotais = 0;
            //Caso o número máximo de episódios não seja conhecido, não deve existir um limite;
            if(listaTemp[lposicao]->vnumEpisodiosTotais != "?")
                lepisodiosTotais = listaTemp[lposicao]->vnumEpisodiosTotais.toInt();
            else
                lepisodiosTotais = INT_MAX;
            if(listaTemp[lposicao]->vnumEpisodiosAssistidos.toInt() < lepisodiosTotais){
                listaTemp[lposicao]->vnumEpisodiosAssistidos =
                        QString::number(listaTemp[lposicao]->vnumEpisodiosAssistidos.toInt()+1);
                cdatabase->instance()->fmudaProgresso(listaTemp[i]->vid,
                                               listaTemp[i]->vnumEpisodiosAssistidos);
                finfoAnimeSelecionado(vposicaoGridAnimeSelecionado);
                QString lacao = "progresso:" + listaTemp[lposicao]->vid;
                QStringList lstringListAcao = lacao.split(':');
                vlistaAcoes.insert(lstringListAcao,
                                   QString::number(listaTemp[lposicao]->vnumEpisodiosAssistidos.toInt()));
                if(listaTemp[lposicao]->vnumEpisodiosAssistidos.toInt() ==
                        listaTemp[lposicao]->vnumEpisodiosTotais.toInt()){
                    listaTemp[lposicao]->vlista = "Completed";
                    cclient->fmudaLista(listaTemp[lposicao]->vid.toInt(), "COMPLETED");
                    cdatabase->instance()->fmudaLista(listaTemp[lposicao]->vid,
                                                   listaTemp[lposicao]->vlista, Database::type::ANIME);
                }
                //Caso esteja vendo um anime de, por exemplo, plan to watch, manda pra lista watching
                //Se estiver vendo um anime que não está na lista, ele é adicionado em Watching também.
                else if((llistaAnimeAssistindo.compare("Completed",Qt::CaseInsensitive) != 0)){
                    listaTemp[lposicao]->vlista = "Watching";
                    cclient->fmudaLista(listaTemp[lposicao]->vid.toInt(), "CURRENT");
                    cdatabase->instance()->fmudaLista(listaTemp[lposicao]->vid,
                                                   listaTemp[lposicao]->vlista, Database::type::ANIME);
                }
            }
        }
    }
    //Aqui rola o refresh depois de ter mudado a lista temporária
    if(llistaAnimeAssistindo.compare("On Hold",Qt::CaseInsensitive) == 0){
        vlistaSelecionada = cdatabase->instance()->returnSortList(vordemLista, "PAUSED", Database::type::ANIME);
    }
    else if(llistaAnimeAssistindo.compare("Dropped",Qt::CaseInsensitive) == 0){
        vlistaSelecionada = cdatabase->instance()->returnSortList(vordemLista, "DROPPED", Database::type::ANIME);
    }
    else if(llistaAnimeAssistindo.compare("Plan to Watch",Qt::CaseInsensitive) == 0){
        vlistaSelecionada = cdatabase->instance()->returnSortList(vordemLista, "PLANNING", Database::type::ANIME);
    }
    else if(llistaAnimeAssistindo.compare("Completed",Qt::CaseInsensitive) == 0){
        vlistaSelecionada = cdatabase->instance()->returnSortList(vordemLista, "COMPLETED", Database::type::ANIME);
    }
    else{
        vlistaSelecionada = cdatabase->instance()->returnSortList(vordemLista, "CURRENT", Database::type::ANIME);
    }
    if(llistaAtual.compare(llistaAnimeAssistindo, Qt::CaseInsensitive) == 0){
        if(vlistaSelecionada.size() > vindexAnimeSelecionado && vindexAnimeSelecionado != 0){
            vindexAnimeSelecionado--;
            if((vindexAnimeSelecionado < 12*(vpagina-1)) && vpagina != 1){
                vpagina--;
            }
        }
        else{
            vindexAnimeSelecionado = 0;
            vpagina = 1;
        }
    }
}

void MainClass::fmudaProgresso(QVariant progresso)
{
    if(progresso.toString().compare("diminui") == 0){
        if(vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosAssistidos.toInt() > 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosAssistidos =
                    QString::number(vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosAssistidos.toInt()-1);
            cdatabase->instance()->fmudaProgresso(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosAssistidos);
            finfoAnimeSelecionado(vposicaoGridAnimeSelecionado);
            QString lacao = "progresso:" + vlistaSelecionada[vindexAnimeSelecionado]->vid;
            QStringList lstringListAcao = lacao.split(':');
            vlistaAcoes.insert(lstringListAcao,
                               QString::number(vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosAssistidos.toInt()));
        }
    }
    else if(progresso.toString().compare("aumenta") == 0){
        int lepisodiosTotais = 0;
        //Caso o número máximo de episódios não seja conhecido, não deve existir um limite;
        if(vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosTotais != "?")
            lepisodiosTotais = vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosTotais.toInt();
        else
            lepisodiosTotais = INT_MAX;
        if(vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosAssistidos.toInt() < lepisodiosTotais){
            vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosAssistidos =
                    QString::number(vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosAssistidos.toInt()+1);
            cdatabase->instance()->fmudaProgresso(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosAssistidos);
            finfoAnimeSelecionado(vposicaoGridAnimeSelecionado);
            QString lacao = "progresso:" + vlistaSelecionada[vindexAnimeSelecionado]->vid;
            QStringList lstringListAcao = lacao.split(':');
            vlistaAcoes.insert(lstringListAcao,
                               QString::number(vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosAssistidos.toInt()));
            if(vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosAssistidos.toInt() ==
                    vlistaSelecionada[vindexAnimeSelecionado]->vnumEpisodiosTotais.toInt()){
                fmudaListaAnime("COMPLETED");
            }
            else if((vlistaSelecionada[vindexAnimeSelecionado]->vlista.compare("Completed",Qt::CaseInsensitive) != 0)){
                vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Watchig";
                cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "CURRENT");
                cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                               vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
            }
        }
    }
}

void MainClass::fmudaNota(QVariant nota)
{
    if(nota.toString().compare("diminui") == 0){
        if(vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaPessoal.toInt() > 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaPessoal =
                    QString::number(vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaPessoal.toInt()-1);
            emit smediaPessoalAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaPessoal));
            cdatabase->instance()->fmudaNota(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaPessoal);
            finfoAnimeSelecionado(vposicaoGridAnimeSelecionado);
            QString lacao = "nota:" + vlistaSelecionada[vindexAnimeSelecionado]->vid;
            QStringList lstringListAcao = lacao.split(':');
            vlistaAcoes.insert(lstringListAcao,
                               QString::number(vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaPessoal.toInt()));
        }
    }
    else if(nota.toString().compare("aumenta") == 0){
        if(vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaPessoal.toInt() < 10){
            vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaPessoal =
                    QString::number(vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaPessoal.toInt()+1);
            emit smediaPessoalAnimeSelecionado(QVariant(vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaPessoal));
            cdatabase->instance()->fmudaNota(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaPessoal);
            finfoAnimeSelecionado(vposicaoGridAnimeSelecionado);
            QString lacao = "nota:" + vlistaSelecionada[vindexAnimeSelecionado]->vid;
            QStringList lstringListAcao = lacao.split(':');
            vlistaAcoes.insert(lstringListAcao,
                               QString::number(vlistaSelecionada[vindexAnimeSelecionado]->vnotaMediaPessoal.toInt()));
        }
    }
}

void MainClass::fresetRequests()
{
    vrateLimitRequests = 0;
    timerMaxClientRequests.singleShot(61000, this, &MainClass::fresetRequests);
}

void MainClass::fadicionaNomeAlternativo(QVariant animeNovoNome)
{
    //caso retorne true, colocar no AVISO
    if(!animeNovoNome.toString().isEmpty())
        cdatabase->instance()->finsereNomeAlternativo(vlistaSelecionada[vindexAnimeSelecionado]->vid, animeNovoNome.toStringList());
}

void MainClass::fselecionaPastaespecificaAnime(QVariant dir)
{
    if(dir.toString() != ""){
        cconfiguracoesUsuarioDiretorios->instance()->fselecionaPastaEspecificaAnime(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                                                                    dir.toString());
    }
}

void MainClass::fremoveFromList()
{
//    cclient->fexcluiAnime(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt());
    vordemLista = "";
    QString lacao = "remove:" + vlistaSelecionada[vindexAnimeSelecionado]->vid;
    QStringList lstringListAcao = lacao.split(':');
    vlistaAcoes.insert(lstringListAcao, "null");
    cdatabase->instance()->fdeletedaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid);
    vlistaSelecionada = cdatabase->instance()->returnSortList(vordemLista, vlistaAtual, vtipoAtual);
    if(vlistaSelecionada.size() > vindexAnimeSelecionado && vindexAnimeSelecionado != 0){
        vindexAnimeSelecionado--;
        if(vindexAnimeSelecionado < 12*(vpagina-1) && vpagina != 1){
            vpagina--;
        }
    }
    else
        vindexAnimeSelecionado = -1;
    finfoAnimeSelecionado(vposicaoGridAnimeSelecionado);
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

void MainClass::fmudaListaAnime(QVariant rnewListVariant)
{
    QString rnewList = rnewListVariant.toString();
    //Check if the user is trying to change the entry from list A to list A
    if(rnewList.compare(vlistaAtual, Qt::CaseInsensitive) == 0)
        return;
    switch (vtipoAtual) {
    case Database::type::ANIME:
        if(rnewList.compare("CURRENT") == 0){
            //Change the anime info
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Watching";
            //Update anime client
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "CURRENT");
            //Update local variables
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("COMPLETED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Completed";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "COMPLETED");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PAUSED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "On Hold";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PAUSED");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("DROPPED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Dropped";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "DROPPED");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PLANNING") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Plan to Watch";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PLANNING");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        break;
    case Database::type::MANGA:
        if(rnewList.compare("CURRENT") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Reading(Manga)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "CURRENT");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("COMPLETED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Completed(Manga)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "COMPLETED");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PAUSED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "On Hold(Manga)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PAUSED");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("DROPPED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Dropped(Manga)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "DROPPED");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PLANNING") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Plan to Read(Manga)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PLANNING");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        break;
    case Database::type::NOVEL:
        if(rnewList.compare("CURRENT") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Reading(Novel)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "CURRENT");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("COMPLETED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Completed(Novel)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "COMPLETED");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PAUSED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "On Hold(Novel)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PAUSED");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("DROPPED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Dropped(Novel)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "DROPPED");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        else if(rnewList.compare("PLANNING") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Plan to Read(Novel)";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PLANNING");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, vtipoAtual);
        }
        break;
    case Database::type::SEASON:
        if(rnewList.compare("CURRENT") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Watching";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "CURRENT");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, Database::type::ANIME);
        }
        else if(rnewList.compare("COMPLETED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Completed";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "COMPLETED");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, Database::type::ANIME);
        }
        else if(rnewList.compare("PAUSED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "On Hold";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PAUSED");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, Database::type::ANIME);
        }
        else if(rnewList.compare("DROPPED") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Dropped";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "DROPPED");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, Database::type::ANIME);
        }
        else if(rnewList.compare("PLANNING") == 0){
            vlistaSelecionada[vindexAnimeSelecionado]->vlista = "Plan to Read";
            cclient->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid.toInt(), "PLANNING");
            cdatabase->instance()->fmudaLista(vlistaSelecionada[vindexAnimeSelecionado]->vid,
                                           vlistaSelecionada[vindexAnimeSelecionado]->vlista, Database::type::ANIME);
        }
        break;
    default:
        break;
    }

    //Update de GUI with the new, change, lists
    vlistaSelecionada = cdatabase->instance()->returnSortList(vordemLista, vlistaAtual, vtipoAtual);
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
