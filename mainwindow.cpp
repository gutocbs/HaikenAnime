#include "mainwindow.h"
#include "ui_mainwindow.h"
#define thisWid this->findChild<QWidget *>(wid->objectName())

//ROBOT_NS_USE_ALL;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    fontName = "\"Tahoma\"";
    fontSizeTitulo = "11";
    fontSizeTituloAlternativo = "10";
    fontSizeStatus = "14";
    fontSizeInfo = "12";
    fontSizeMensagem = "10";
    fontSizeMensagemAnime = "8";
    fatualizaFontes();
    vtituloNormalMedio = "background: transparent; font: 75 7pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);";
    vtituloNormalPequeno = "background: transparent; font: 75 6pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);";
    vfonteNormal = "background: transparent; font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);";
    vfonteNormalNota = "background: transparent; font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(53, 68, 120);";
    vfonteNormalAnimeEncontrado = "background: transparent; font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(53, 120, 56);";
    vfonteNormalAnimeNaoComecou = "background: transparent; font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(120, 60, 53);";
    vfonteTitulo = "background: transparent;font: 75 "+fontSizeTitulo+"pt " + fontName +
            ";font-weight: bold; color: rgb(20, 20, 20);";
    vfonteSubtitulo = "background: transparent;font: 75 "+fontSizeTituloAlternativo+"pt " + fontName +
            "; color: rgb(20, 20, 20);";
    vfonteStatus = "background: transparent;font: 75 "+fontSizeStatus+"pt " + fontName +
            "; color: rgb(20, 20, 20);";
    vfonteInfo = "background: transparent;font: 75 "+fontSizeInfo+"pt " + fontName +
            ";font-weight: bold; color: rgb(20, 20, 20);";
    vfonteMensagem = "background: transparent;font: 75 "+fontSizeMensagem+"pt " + fontName +
            ";color: rgb(20, 20, 20);";
    vfonteMensagemAnime = "background: transparent;font: 75 "+fontSizeMensagemAnime+"pt " + fontName +
            ";color: rgb(20, 20, 20);";
    ui->botaoDownloadListImages->hide();
    ui->boxSelecionaAno->hide();
    ui->botaoAnimeAssistindoCerto->hide();
    if(ui->boxSelecionaAno->count() != QDate::currentDate().year()-2000){
        int date = QDate::currentDate().year()-1999;
        for(int i = 0; i < date; i++){
            ui->boxSelecionaAno->addItem(QString::number(2000+i));
        }
        ui->boxSelecionaAno->setCurrentIndex(QDate::currentDate().year()-2000);
        connect(ui->boxSelecionaAno, QOverload<int>::of(&QComboBox::currentIndexChanged),[=](){
            on_botaoSeason_clicked();
        });
    }
    ui->barraBusca->setMaximumBlockCount(1);
    ui->barraAddNomeAlternativo->setMaximumBlockCount(1);
    ///um dia ainda faço qml disso
    fmudaResolucao();
    logger::fattachLogger();
    qInfo() << QDateTime::currentDateTime().toString();
    vusarImagensBaixaQualidade = false;
    canilist = new anilist(nullptr);
    canilist->frecebeAutorizacao(jconfig.fretornaUsuario(),jconfig.fretornaCodigoAutorizacao());
    cconfBase = new confBase(nullptr);
    cconfBase->fcriaDiretoriosBase();
    dirGrande = cconfBase->vdiretorioImagensGrandes;
    dirMedio = cconfBase->vdiretorioImagensMedio;
    dirPequeno = cconfBase->vdiretorioImagensPequenas;
    if(dirGrande.isEmpty())
        dirGrande = "0";
    if(dirPequeno.isEmpty())
        dirPequeno = "0";
    if(dirMedio.isEmpty())
        dirMedio = "0";
    fgetConfigurations();
    qDebug() << "Main configuration is up";
    carquivos = new arquivos(this);
    qDebug() << "File system is up";
    cconfUsuario = new confUsuario(nullptr);
    qDebug() << "User configuration is up";

    cleitorListaAnimes = new leitorlistaanimes(nullptr);
    cfiledownloader = new filedownloader(nullptr);
    vtimerSegundos = 59;
    vcontadorAssistindoEpisodio = 0;

    //E o timer para atualizar a lista automaticamente
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::on_botaoRefresh_clicked));
    //O segundo timer é uma contagem de segundo em segundo pra mostrar quanto tempo falta para o timer de cima chegar à 0
    timerRefresh = new QTimer(this);
    connect(timerRefresh, &QTimer::timeout, this, QOverload<>::of(&MainWindow::fatualizaRefreshTimer));
    tryTimer = new QTimer(this);
    tryTimer->start(30000);
    connect(tryTimer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::fatualizaAnilist));
    timerChecaAnimes = new QTimer(this);
    timerChecaAnimes->start(10000);
    connect(timerChecaAnimes, &QTimer::timeout, this, QOverload<>::of(&MainWindow::fVerificaAnimeAberto));
    timerChecaDownloadPorAno = new QTimer(this);
    connect(timerChecaDownloadPorAno, &QTimer::timeout, this, QOverload<>::of(&MainWindow::fsetDownloadImagensAnimesPorAno));
    timerTorrent = new QTimer(this);
    ui->janelaRotativa->addWidget(&jconfig);
    ui->janelaRotativa->addWidget(&jtorrent);
    jtorrent.fpassaPonteiros(cleitorListaAnimes, &jconfig, carquivos);
    jconfig.frecebeListasAnimes(cleitorListaAnimes);
    connect(&jtorrent, &janelatorrent::error, this, &MainWindow::favisoErro);

    vordem = "";
    vdownloadImagensAcabou = true;
    //Prepara as threads de download.
    canilist->fbaixaListaThread(cThread);
    canilist->moveToThread(&cThread);
    cconfUsuario->fbuscaPastasThread(dThread);
    cconfUsuario->moveToThread(&dThread);

    //jtorrent e jconfig
    connect(canilist, &anilist::sterminouDownload, this, &MainWindow::fcarregouListaTeste, Qt::QueuedConnection);
    connect(&jconfig, &janeladeconfig::sauthcodesave, this, &MainWindow::fretryAnilist);
    connect(&jconfig, &janeladeconfig::ssavebutton, this, &MainWindow::fretryAnilist);
    connect(&jconfig, &janeladeconfig::ssavebutton, this, &MainWindow::fgetConfigurations);
    connect(&jtorrent, &janelatorrent::infoAnime, this, &MainWindow::fInfoAnimeTorrent, Qt::QueuedConnection);

    //Quando baixa todas as imagens, carrega as imagens na página
    connect(cfiledownloader,&filedownloader::sterminouLista,this,&MainWindow::fcarregaImagensBackground, Qt::QueuedConnection);
    connect(cfiledownloader,&filedownloader::sid,this,&MainWindow::fsetIdBaixado, Qt::QueuedConnection);
    connect(cleitorListaAnimes, &leitorlistaanimes::sAnimeAdicionadoNaLista, this, &MainWindow::fsetIdAdicionado, Qt::QueuedConnection);
    connect(cconfUsuario, &confUsuario::schecouPastas, this, &MainWindow::fmandaDiretoriosArquivos, Qt::QueuedConnection);
    connect(this, &MainWindow::sterminouCarregarImagens, this, &MainWindow::fcarregaImagensLista, Qt::QueuedConnection);
    connect(this, &MainWindow::sanimeReconhecido, this, &MainWindow::fMostraAnimeAberto, Qt::QueuedConnection);

    //Se a lista foi carregada com sucesso, temos certeza de que o usuário já foi lido.
    this->setWindowTitle("Atom - " + jconfig.fretornaUsuario());
    ui->labelUsername->setText(jconfig.fretornaUsuario());
    //Caso tenha lido a lista com sucesso, começa a procurar pelos animes no computador.
    vlistaAtual = "watching";
    vtipoAtual = "anime";
    // Já aciona a lista de modo ordenado
    vordem = "";
    cconfUsuario->flePastasArquivos();
    fmandaDiretoriosArquivos();
    cfiledownloader->fsetConfBase(cconfBase);
    cfiledownloader->fsetLeitorListaAnimes(cleitorListaAnimes);
//    connect(cfiledownloader, &filedownloader::savatar, this, &MainWindow::fcarregaImagensLista, Qt::QueuedConnection);

    vanimeSelecionado = 0;
    vpagina = 1;
    fleNomesAlternativos();
    fcarregouListaTeste(false);

    //Caso haja algum erro onde não foi possível atualizar o anilist, as ações não atualizadas são salvas em um bloco de
    //texto. Ao iniciar o programa, é verificado se existe alguma atualização que não foi completada ainda
    QFile lfile("Configurações/Temp/ListUpdates.txt");
    if(lfile.exists() && lfile.size() != 0){
        if(lfile.open(QIODevice::ReadOnly)) {
            QTextStream lstreamTexto(&lfile);
            lstreamTexto.setCodec("UTF-8");
            while(!lstreamTexto.atEnd()){
                QStringList linha = lstreamTexto.readLine().split("-");
                QString act = linha.takeLast();
                vlistaAcoes.insert(linha, act);
            }
            lfile.close();
        }
    }
    QFile lfileHistorico("Configurações/Temp/Historico.txt");
    if(lfileHistorico.exists() && lfileHistorico.size() != 0){
        if(lfileHistorico.open(QIODevice::ReadOnly)) {
            QTextStream lstreamTexto(&lfileHistorico);
            lstreamTexto.setCodec("UTF-8");
            while(!lstreamTexto.atEnd()){
                QString linha = lstreamTexto.readLine().simplified();
                vhistoricoAnilist.append(linha);
            }
            lfileHistorico.close();
        }
    }
    jconfig.fsetHistorico(vhistoricoAnilist);
    vhistoricoAnilist.clear();
    cleitorListaAnimes->fleListaIdsNomesAnos();
    vprogressoLoadingBar = 0;
    vtotalAnimesLoadingBar = 0;
    vcontadorBotaoLoadImages = 0;
    vrateLimitRequestsAnilist = 0;
    vanoBuscaAnimes = 0;
    ui->labelMensagemAnimeAssistindo->setStyleSheet(vfonteMensagemAnime);
    fresetaRequestCounter();
}

MainWindow::~MainWindow()
{
    if(!vlistaAcoes.isEmpty()){
        QFile lfile("Configurações/Temp/ListUpdates.txt");
        if(lfile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            QTextStream lstreamTexto(&lfile);
            lstreamTexto.setCodec("UTF-8");
            foreach (QStringList key, vlistaAcoes.keys()){
                lstreamTexto << key.at(0) << "-" << key.at(1) << "-" << vlistaAcoes[key] << endl;
                vlistaAcoes.remove(key);
            }
            lstreamTexto.flush();
            lfile.close();
        }
    }
    if(!vhistoricoAnilist.isEmpty()){
        QFile lfile("Configurações/Temp/Historico.txt");
        if(lfile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            QTextStream lstreamTexto(&lfile);
            lstreamTexto.setCodec("UTF-8");
            foreach (QString historico, vhistoricoAnilist){
                lstreamTexto << historico << endl;
            }
            lstreamTexto.flush();
            lfile.close();
        }
    }
    jconfig.fsalvaFiltros();
    fsalvaNomesAlternativos();
    cleitorListaAnimes->fsalvaListaIdsNomesAnos();
    vrefreshAcontecendo = true;
    if(vcarregaImagens.isRunning())
        vcarregaImagens.waitForFinished();
    cThread.requestInterruption();
    cThread.wait();
    dThread.requestInterruption();
    dThread.wait();
    qDebug() << "Deleting pointers";
    cconfUsuario->deleteLater();
    qDebug() << "cconfUsuario deleted";
    carquivos->deleteLater();
    qDebug() << "carquivos deleted";
    timer->deleteLater();
    timerRefresh->deleteLater();
    tryTimer->deleteLater();
    cfiledownloader->deleteLater();
    qDebug() << "cfiledownloader deleted";
    cleitorListaAnimes->deleteLater();
    qDebug() << "cleitorListaAnimes deleted";
    canilist->deleteLater();
    qDebug() << "canilist deleted";
    qInfo() << "Software closed successfully";
    delete ui;
}

void MainWindow::fupdateMessage(const QString &mensagem)
{
    ui->labelMensagem->setText(mensagem);
}

void MainWindow::fcarregouListaTeste(bool ldownload){
    fbloqueiaSinaisBotoes();
    fsalvaNomesAlternativos();
    cleitorListaAnimes->fdeletaListaAnimes();
    vlistaLidaSucesso = cleitorListaAnimes->fleJson();
    if(vlistaLidaSucesso){
        fliberaSinaisBotoes();
        fcarregouListaSucesso(ldownload);
    }
    else{
        qDebug() << "Failed to read the anime list.";
        fupdateMessage("Failed to read the anime list");
        fcarregouListaFalha();
    }
}


void MainWindow::fcarregouListaSucesso(bool ldownload){
    //Ele sempre vai tentar baixar a lista. Caso tenha baixado, isso vira true e não é necessário baixar de novo
    //Caso não tenha baixado a lista ainda ou deu algum erro no download, tenta baixar a lista de novo.
    if(!ldownload)
        fcarregouListaFalha();
    else{
        fupdateMessage("Downloaded successfully");
        vlistaBaixada = true;
    }
    qDebug() << "The anime list have been read successfully";
    fleNomesAlternativos();
    //Atualiza o avatar do usuário também.
    qDebug() << "The download system is up";
    vprogressoLoadingBar = 0;
    cfiledownloader->fresetCounters();
    cfiledownloader->fsetDownload(vusarImagensBaixaQualidade);
    vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, vlistaAtual);

    qDebug() << "Searching for animes in the computer";
    //Posso por o recebe configs em uma função própria com sinal de qunado salvar as configs
    cconfUsuario->frecebeConfigs(jconfig.fretornaDiretorios());
    carquivos->frecebeAnimes(cleitorListaAnimes);
    if(vlistaSelecionada.size() < vanimeSelecionado){
        vanimeSelecionado = 0;
        vpagina = 1;
    }

    ui->NumPagina->setText("Watching - " + QString::number(vlistaSelecionada.size()) +
                           " animes in the list - Page "+QString::number(vpagina)+"/"+
                           QString::number(((vlistaSelecionada.size()-1)/12)+1));

    if(ldownload){
        vrefreshAcontecendo = true;
        frefreshListas(ldownload);
    }
    finfoAnimeSelecionado();
}

void MainWindow::fcarregouListaFalha(){
    vlistaBaixada = false;
    qDebug() << "Checking connection with Anilist";
    cThread.start();
//    canilist->fgetList();
    qDebug() << "Trying to download lists";
    fupdateMessage("Trying to download the anime lists. The application may freeze "
                               "for a few seconds while updating.");
}

void MainWindow::fsetIdAdicionado(const QString &lid)
{
    if(!vimagemBaixada.contains(lid)){
        vtotalAnimesLoadingBar++;
        vimagemBaixada.insert(lid, false);
        vimagemCarregada.insert(lid, false);
        vimagemBaixadaGrande.insert(lid, false);
        vimagemCarregadaGrande.insert(lid, false);
    }
}

void MainWindow::fsetIdBaixado(const QString &lid)
{
    if(vprogressoLoadingBar != vtotalAnimesLoadingBar)
        vprogressoLoadingBar++;
    ui->labelLoading->setText("Downloaded " + QString::number(vprogressoLoadingBar) + " of " +
                              QString::number(vtotalAnimesLoadingBar) + " images");
    vimagemBaixada[lid] = true;
}

void MainWindow::fsetIdBaixadoGrande(const QString &lid)
{
    vimagemBaixadaGrande[lid] = true;
}

void MainWindow::fInfoAnimeTorrent(const QString &lnomeAnime)
{
    ui->janelaRotativa->setCurrentIndex(0);
    ui->barraBusca->setPlainText(lnomeAnime);
    on_botaoBusca_clicked();
}

void MainWindow::fVerificaAnimeAberto()
{
    //Se eu fizer um ponteiro disso, o ponteiro vai poder ser deletado sem fechar o programa na memória?
    Robot::Window *llistaJanelas = new Robot::Window;
    QString nomejanela;
    auto vlistaJanelas = llistaJanelas->GetList();
    foreach (const auto& janela, vlistaJanelas) {
        if(vrefreshAcontecendo)
            return;
        if(!QString::fromStdString(janela.GetTitle()).isEmpty()){
            nomejanela = QString::fromStdString(janela.GetTitle());
            foreach(QString player, vPlayers){
                if(vrefreshAcontecendo)
                    return;
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
                if(nomejanela.contains(player, Qt::CaseInsensitive)){
                    QPointer<ChecaInfoPlayer> c(new ChecaInfoPlayer);
                    c->fchecaStream(player, nomejanela);
                    QString nomeAnime = c->fretornaAnime();
                    QString episodio = c->fretornaEpisodio();
                    QString vidAnime = cleitorListaAnimes->fprocuraAnimeNasListas(nomeAnime);
//                    qDebug() << vidAnime << nomeAnime << episodio;
                    if(!vidAnime.isEmpty()){
                        emit sanimeReconhecido(vidAnime, nomeAnime, episodio);
                        delete llistaJanelas;
                        return;
                    }
                    else{
                        vnomeAnimeAberto = nomeAnime;
                        ui->labelAnimeAssistindoTitulo->setAlignment(Qt::AlignCenter);
                        ui->labelAnimeAssistindoEpisodio->setAlignment(Qt::AlignCenter);
                        ui->labelAnimeAssistindoTitulo->setWordWrap(true);
                        ui->labelAnimeAssistindoTitulo->setText(nomeAnime);
                        ui->labelAnimeAssistindoEpisodio->setStyleSheet(vfonteNormal);
                        ui->labelAnimeAssistindoEpisodio->setText("Episode not recognized.");
                        ui->labelMensagemAnimeAssistindo->setText("If the anime wasn't recognized correctly, search for the right"
                        " anime and click here to change!");
                        ui->botaoAnimeAssistindoCerto->show();
                        delete llistaJanelas;
                        return;
                    }
                }
            }
            ui->imagemAnimeAssistindo->clear();
            ui->labelAnimeAssistindoTitulo->clear();
            ui->labelAnimeAssistindoEpisodio->clear();
            ui->labelFundoAnimeAssistindoTitulo->setStyleSheet("background: transparent");
            ui->labelFundoAnimeAssistindoEpisodio->setStyleSheet("background: transparent");
            ui->botaoAnimeAssistindo->blockSignals(true);
        }
    }
    vcontadorAssistindoEpisodio = 0;
    vnomeAnimeAberto = "";
    ui->labelMensagemAnimeAssistindo->clear();
    ui->botaoAnimeAssistindoCerto->hide();
    delete llistaJanelas;
}

void MainWindow::fMostraAnimeAberto(QString ridAnime, const QString &rnomeAnime, const QString &repisodioAnime)
{
    vnomeAnimeAberto = rnomeAnime;
    idAnimeAssistindo = ridAnime;
    ui->botaoAnimeAssistindoCerto->show();
    ui->labelMensagemAnimeAssistindo->setText("If the anime wasn't recognized correctly, search for the right"
    " anime and click here to change!");
    ui->imagemAnimeAssistindo->setScaledContents(true);
    ui->botaoAnimeAssistindo->blockSignals(false);
    QPixmap lpix;
    if(vimagemCarregada[ridAnime]){
        if(QFile::exists(dirMedio+ridAnime+".jpg")){
            if(lpix.load(dirMedio+ridAnime+".jpg", "jpg", Qt::ColorOnly)){
                ui->imagemAnimeAssistindo->setPixmap(lpix);
            }
        }
        else if(QFile::exists(dirMedio+ridAnime+".png")){
            if(lpix.load(dirMedio+ridAnime+".png", "png", Qt::ColorOnly)){
                ui->imagemAnimeAssistindo->setPixmap(lpix);
            }
        }
    }
    if(rnomeAnime.size() < 47)
        ui->labelAnimeAssistindoTitulo->setStyleSheet(vfonteNormal);
    else if(rnomeAnime.size() < 58)
        ui->labelAnimeAssistindoTitulo->setStyleSheet(vtituloNormalMedio);
    else
        ui->labelAnimeAssistindoTitulo->setStyleSheet(vtituloNormalPequeno);
    ui->labelAnimeAssistindoEpisodio->setStyleSheet(vfonteNormal);
    ui->labelAnimeAssistindoTitulo->setText(rnomeAnime);
    ui->labelAnimeAssistindoEpisodio->setText("Episode " + repisodioAnime);
    ui->labelFundoAnimeAssistindoTitulo->setStyleSheet("background: rgb(185,201,250);");
    ui->labelFundoAnimeAssistindoEpisodio->setStyleSheet("background: rgb(185,201,250);");
    ui->labelAnimeAssistindoTitulo->setAlignment(Qt::AlignCenter);
    ui->labelAnimeAssistindoEpisodio->setAlignment(Qt::AlignCenter);
    ui->labelAnimeAssistindoTitulo->setWordWrap(true);

    //Vai checar a cada 10 segundos, então tem que assistir dois minutos de anime pra contar. Assim, atualiza os curtas ao mesmo
    //tempo que não atualiza se você mal assistir
    if(vcontadorAssistindoEpisodio == 20){
        vcontadorAssistindoEpisodio++;
        fAumentaProgressoID(ridAnime, repisodioAnime);
    }
    else if(vcontadorAssistindoEpisodio == 21)
        return;
    else
        vcontadorAssistindoEpisodio++;
    finfoAnimeSelecionado();
}

///ASSIM QUE O DESIGN ESTIVER PRONTO, CHECAR TAMBÉM SE A SINOPSE É MAIOR QUE O LABEL E AJEITAR ISSO
void MainWindow::finfoAnimeSelecionado(){
//    fVerificaAnimeAberto();
    if(!vlistaSelecionada.isEmpty() || vanimeSelecionado != -1){
        ui->labelInfoNomeAnime->setText(vlistaSelecionada[vanimeSelecionado]->vnome);
        ui->labelInfoNomeAnimeIngles->setText(vlistaSelecionada[vanimeSelecionado]->vnomeIngles);
        ui->labelInfoSinopse->setText(vlistaSelecionada[vanimeSelecionado]->vsinopse);
        ui->labelInfoStatus->setText(vlistaSelecionada[vanimeSelecionado]->vstatus);
        ui->labelInfoSeason->setText(vlistaSelecionada[vanimeSelecionado]->vseason);
        ui->labelInfoMediaSite->setText(QString::number(vlistaSelecionada[vanimeSelecionado]->vnotaMediaSite.toDouble()/10, 'f',2));
        ui->labelInfoNota->setText(vlistaSelecionada[vanimeSelecionado]->vnotaMediaPessoal);
        ui->labelInfoRelease->setText(vlistaSelecionada[vanimeSelecionado]->vdataEpisodio);
        if(vlistaSelecionada[vanimeSelecionado]->vnumProximoEpisodioLancado == "" && vlistaSelecionada[vanimeSelecionado]->vstatus
                != "Finished Airing")
            ui->labelInfoEpisodiosLancados->setText("-");
        else if(vlistaSelecionada[vanimeSelecionado]->vnumProximoEpisodioLancado.toInt() == 0)
            ui->labelInfoEpisodiosLancados->setText("All Episodes");
        else
            ui->labelInfoEpisodiosLancados->setText(QString::number(vlistaSelecionada[vanimeSelecionado]->
                                                                    vnumProximoEpisodioLancado.toInt() - 1) + " Episodes");
        ui->labelInfoTipo->setText(vlistaSelecionada[vanimeSelecionado]->vformato);
        ui->labelInfoNota2->setText(vlistaSelecionada[vanimeSelecionado]->vnotaMediaPessoal);
        ui->labelInfoProgresso->setText(vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosAssistidos +
                                        "/" + vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosTotais);
    }
    else{
        ui->labelInfoNomeAnime->clear();
        ui->labelInfoNomeAnimeIngles->clear();
        ui->labelInfoSinopse->clear();
        ui->labelInfoStatus->clear();
        ui->labelInfoSeason->clear();
        ui->labelInfoMediaSite->clear();
        ui->labelInfoNota->clear();
        ui->labelInfoRelease->clear();
        ui->labelInfoEpisodiosLancados->clear();
        ui->labelInfoTipo->clear();
    }
    fcarregaImagensLista();
}

///BOTAR ISSO NUM LOOP, PFV, TEM 675 LINHAS SÓ NESSA FUNÇÃO!
/// //tentei e não deu ::sad face::
/// Mas tudo bem, vou pensar em algo
void MainWindow::fcarregaImagensLista(){
    QPixmap pix;
    //Tenta carrgar o avatar antes de tudo
    ui->labelAvatar->setScaledContents(true);
    if(pix.load("Configurações/Temp/Imagens/avatar.jpg", "jpg")){
        ui->labelAvatar->setPixmap(pix);
    }
    if(!vlistaSelecionada.isEmpty()){
        ui->imagemAnime00->setScaledContents(true);
        ui->imagemAnimeGrande->setScaledContents(true);
        //Tenta carregar a imagem como jpg, se possível
        if(pix.load(cconfBase->vdiretorioImagensGrandes+vlistaSelecionada[vanimeSelecionado]->vid+".jpg", "jpg")){
            ui->imagemAnimeGrande->setPixmap(pix);
        }
        //Se não conseguir carregar como jpg, tenta carregar como png
        else if(pix.load(cconfBase->vdiretorioImagensGrandes+vlistaSelecionada[vanimeSelecionado]->vid+".png", "png")){
            ui->imagemAnimeGrande->setPixmap(pix);
        }
        //Se a imagem grande não existir, vai carregar a imagem média no lugar
        else if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[vanimeSelecionado]->vid+".jpg", "jpg")){
            ui->imagemAnimeGrande->setPixmap(pix);
        }
        else if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[vanimeSelecionado]->vid+".png", "png")){
            ui->imagemAnimeGrande->setPixmap(pix);
        }
        else{
            if(pix.load(cconfBase->vimagemStaple, "jpg"))
                ui->imagemAnimeGrande->setPixmap(pix);
        }
        if(vlistaSelecionada[0+(12*(vpagina-1))]->vnome.size() < 47)
            ui->labelAnime00Titulo->setStyleSheet(vfonteNormal);
        else if(vlistaSelecionada[0+(12*(vpagina-1))]->vnome.size() < 58)
            ui->labelAnime00Titulo->setStyleSheet(vtituloNormalMedio);
        else
            ui->labelAnime00Titulo->setStyleSheet(vtituloNormalPequeno);
        ui->labelFundoAnime00Nota->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime00Progresso->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime00Titulo->setStyleSheet("background: rgb(185,201,250);");
        ui->labelAnime00Titulo->setText(vlistaSelecionada[0+(12*(vpagina-1))]->vnome);
        ui->labelAnime00Titulo->setAlignment(Qt::AlignCenter);
        ui->labelAnime00Titulo->setWordWrap(true);
        ui->labelAnime00Progresso->setAlignment(Qt::AlignCenter);
        ui->labelAnime00Nota->setStyleSheet(vfonteNormalNota);
        if(!carquivos->fprocuraEpisodio(vlistaSelecionada[0+(12*(vpagina-1))]).isEmpty() && vtipoAtual == "anime")
            ui->labelAnime00Progresso->setStyleSheet(vfonteNormalAnimeEncontrado);
        else if(vlistaSelecionada[0+(12*(vpagina-1))]->vstatus.compare("Not Aired Yet", Qt::CaseInsensitive) == 0)
            ui->labelAnime00Progresso->setStyleSheet(vfonteNormalAnimeNaoComecou);
        else
            ui->labelAnime00Progresso->setStyleSheet(vfonteNormal);
        ui->labelAnime00Progresso->setText("Progresso: " + vlistaSelecionada[0+(12*(vpagina-1))]->vnumEpisodiosAssistidos +
                    "/" + vlistaSelecionada[0+(12*(vpagina-1))]->vnumEpisodiosTotais);
        ui->labelAnime00Nota->setAlignment(Qt::AlignCenter);
        ui->labelAnime00Nota->setText("Nota: " + vlistaSelecionada[0+(12*(vpagina-1))]->vnotaMediaPessoal + "/10");
        if(vlistaAtual == "busca" || vlistaAtual.contains("season")){
            ui->labelFundoAnime00Lista->setStyleSheet("background: rgb(185, 202, 250); font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);");
            bool ok;
            //Checa se a lista é um número válido
            vlistaSelecionada[(12*(vpagina-1))]->vlista.toInt(&ok);
            if(ok)
                ui->labelFundoAnime00Lista->setText(vlistaSelecionada[(12*(vpagina-1))]->vSeasonSemAno);
            else
            ui->labelFundoAnime00Lista->setText(vlistaSelecionada[0+(12*(vpagina-1))]->vlista);
        }
        else{
            ui->labelFundoAnime00Lista->clear();
            ui->labelFundoAnime00Lista->setStyleSheet("background: transparent");
        }
        if(vimagemCarregada[vlistaSelecionada[0+(12*(vpagina-1))]->vid]){
            if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[0+(12*(vpagina-1))]->vid+".jpg", "jpg")){
                ui->imagemAnime00->setPixmap(pix);
            }
            else if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[0+(12*(vpagina-1))]->vid+".png", "png")){
                ui->imagemAnime00->setPixmap(pix);
            }
            else{
                ui->imagemAnime00->clear();
                ui->imagemAnime00->setStyleSheet("background: black;");
            }
        }
        else{
            if(pix.load(cconfBase->vimagemStaple, "jpg"))
                ui->imagemAnime00->setPixmap(pix);
        }
    }
    else{

        //PRO LOOP
        //remove imagemAnime remove labelAnime remove labelFundoAnime remove nota remove progresso remove titulo
        //Ai pega o valor que sobrar e joga na condição
        //if valor == 0 && vlistaSelecionada.size() != 0
        //if valor != 0 && vlistaSelecionada.size() > VALOR+(12*(vpagina-1))
        ui->imagemAnimeGrande->clear();
        ui->imagemAnime00->clear();
        ui->imagemAnime00->setStyleSheet("background: transparent;");
        ui->labelAnime00Titulo->clear();
        ui->labelAnime00Progresso->clear();
        ui->labelAnime00Nota->clear();
        ui->labelFundoAnime00Nota->setStyleSheet("background: transparent;");
        ui->labelFundoAnime00Progresso->setStyleSheet("background: transparent;");
        ui->labelFundoAnime00Titulo->setStyleSheet("background: transparent;");
        ui->labelFundoAnime00Lista->clear();
        ui->labelFundoAnime00Lista->setStyleSheet("background: transparent");
    }
    if(vlistaSelecionada.size() > 1+(12*(vpagina-1))){
        ui->imagemAnime01->setScaledContents(true);
        ui->labelFundoAnime01Nota->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime01Progresso->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime01Titulo->setStyleSheet("background: rgb(185,201,250);");
        if(vlistaSelecionada[1+(12*(vpagina-1))]->vnome.size() < 47)
            ui->labelAnime01Titulo->setStyleSheet(vfonteNormal);
        else if(vlistaSelecionada[1+(12*(vpagina-1))]->vnome.size() < 58)
            ui->labelAnime01Titulo->setStyleSheet(vtituloNormalMedio);
        else
            ui->labelAnime01Titulo->setStyleSheet(vtituloNormalPequeno);
        ui->labelAnime01Titulo->setText(vlistaSelecionada[1+(12*(vpagina-1))]->vnome);
        ui->labelAnime01Titulo->setAlignment(Qt::AlignCenter);
        ui->labelAnime01Titulo->setWordWrap(true);
        if(!carquivos->fprocuraEpisodio(vlistaSelecionada[1+(12*(vpagina-1))]).isEmpty() && vtipoAtual == "anime")
            ui->labelAnime01Progresso->setStyleSheet(vfonteNormalAnimeEncontrado);
        else if(vlistaSelecionada[1+(12*(vpagina-1))]->vstatus.compare("Not Aired Yet", Qt::CaseInsensitive) == 0)
            ui->labelAnime01Progresso->setStyleSheet(vfonteNormalAnimeNaoComecou);
        else
            ui->labelAnime01Progresso->setStyleSheet(vfonteNormal);
        ui->labelAnime01Progresso->setAlignment(Qt::AlignCenter);
        ui->labelAnime01Progresso->setText("Progresso: " + vlistaSelecionada[1+(12*(vpagina-1))]->vnumEpisodiosAssistidos +
                    "/" + vlistaSelecionada[1+(12*(vpagina-1))]->vnumEpisodiosTotais);
        ui->labelAnime01Nota->setAlignment(Qt::AlignCenter);
        ui->labelAnime01Nota->setStyleSheet(vfonteNormalNota);
        ui->labelAnime01Nota->setText("Nota: " + vlistaSelecionada[1+(12*(vpagina-1))]->vnotaMediaPessoal + "/10");
        if(vlistaAtual == "busca" || vlistaAtual.contains("season")){
            ui->labelFundoAnime01Lista->setStyleSheet("background: rgb(185, 202, 250); font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);");
            bool ok;
            //Checa se a lista é um número válido
            vlistaSelecionada[1+(12*(vpagina-1))]->vlista.toInt(&ok);
            if(ok)
                ui->labelFundoAnime01Lista->setText(vlistaSelecionada[1+(12*(vpagina-1))]->vSeasonSemAno);
            else
            ui->labelFundoAnime01Lista->setText(vlistaSelecionada[1+(12*(vpagina-1))]->vlista);
        }
        else{
            ui->labelFundoAnime01Lista->clear();
            ui->labelFundoAnime01Lista->setStyleSheet("background: transparent");
        }
        if(vimagemCarregada[vlistaSelecionada[1+(12*(vpagina-1))]->vid]){
            if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[1+(12*(vpagina-1))]->vid+".jpg", "jpg")){
                ui->imagemAnime01->setPixmap(pix);
            }
            else if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[1+(12*(vpagina-1))]->vid+".png", "png")){
                ui->imagemAnime01->setPixmap(pix);
            }
            else{
                ui->imagemAnime01->clear();//Trocar isso por imagem coringa pro caso de não ter imagem do anime
                ui->imagemAnime01->setStyleSheet("background: black;");
            }
        }
        else{
            if(pix.load(cconfBase->vimagemStaple, "jpg"))
                ui->imagemAnime01->setPixmap(pix);
        }
    }
    else{
        ui->imagemAnime01->clear();
        ui->imagemAnime01->setStyleSheet("background: transparent;");
        ui->labelAnime01Titulo->clear();
        ui->labelAnime01Progresso->clear();
        ui->labelAnime01Nota->clear();
        ui->labelFundoAnime01Nota->setStyleSheet("background: transparent;");
        ui->labelFundoAnime01Progresso->setStyleSheet("background: transparent;");
        ui->labelFundoAnime01Titulo->setStyleSheet("background: transparent;");
        ui->labelFundoAnime01Lista->clear();
        ui->labelFundoAnime01Lista->setStyleSheet("background: transparent");
    }
    if(vlistaSelecionada.size() > 2+(12*(vpagina-1))){
        ui->imagemAnime02->setScaledContents(true);
        ui->labelFundoAnime02Nota->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime02Progresso->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime02Titulo->setStyleSheet("background: rgb(185,201,250);");
        if(vlistaSelecionada[2+(12*(vpagina-1))]->vnome.size() < 47)
            ui->labelAnime02Titulo->setStyleSheet(vfonteNormal);
        else if(vlistaSelecionada[2+(12*(vpagina-1))]->vnome.size() < 58)
            ui->labelAnime02Titulo->setStyleSheet(vtituloNormalMedio);
        else
            ui->labelAnime02Titulo->setStyleSheet(vtituloNormalPequeno);
        ui->labelAnime02Titulo->setText(vlistaSelecionada[2+(12*(vpagina-1))]->vnome);
        ui->labelAnime02Titulo->setAlignment(Qt::AlignCenter);
        ui->labelAnime02Titulo->setWordWrap(true);
        ui->labelAnime02Progresso->setAlignment(Qt::AlignCenter);
        if(!carquivos->fprocuraEpisodio(vlistaSelecionada[2+(12*(vpagina-1))]).isEmpty() && vtipoAtual == "anime")
            ui->labelAnime02Progresso->setStyleSheet(vfonteNormalAnimeEncontrado);
        else if(vlistaSelecionada[2+(12*(vpagina-1))]->vstatus.compare("Not Aired Yet", Qt::CaseInsensitive) == 0)
            ui->labelAnime02Progresso->setStyleSheet(vfonteNormalAnimeNaoComecou);
        else
            ui->labelAnime02Progresso->setStyleSheet(vfonteNormal);
        ui->labelAnime02Progresso->setText("Progresso: " + vlistaSelecionada[2+(12*(vpagina-1))]->vnumEpisodiosAssistidos +
                    "/" + vlistaSelecionada[2+(12*(vpagina-1))]->vnumEpisodiosTotais);
        ui->labelAnime02Nota->setAlignment(Qt::AlignCenter);
        ui->labelAnime02Nota->setStyleSheet(vfonteNormalNota);
        ui->labelAnime02Nota->setText("Nota: " + vlistaSelecionada[2+(12*(vpagina-1))]->vnotaMediaPessoal + "/10");
        if(vlistaAtual == "busca" || vlistaAtual.contains("season")){
            ui->labelFundoAnime02Lista->setStyleSheet("background: rgb(185, 202, 250); font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);");
            bool ok;
            //Checa se a lista é um número válido
            vlistaSelecionada[2+(12*(vpagina-1))]->vlista.toInt(&ok);
            if(ok)
                ui->labelFundoAnime02Lista->setText(vlistaSelecionada[2+(12*(vpagina-1))]->vSeasonSemAno);
            else
            ui->labelFundoAnime02Lista->setText(vlistaSelecionada[2+(12*(vpagina-1))]->vlista);
        }
        else{
            ui->labelFundoAnime02Lista->clear();
            ui->labelFundoAnime02Lista->setStyleSheet("background: transparent");
        }
        if(vimagemCarregada[vlistaSelecionada[2+(12*(vpagina-1))]->vid]){
            if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[2+(12*(vpagina-1))]->vid+".jpg", "jpg")){
                ui->imagemAnime02->setPixmap(pix);
            }
            else if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[2+(12*(vpagina-1))]->vid+".png", "png")){
                ui->imagemAnime02->setPixmap(pix);
            }
            else{
                ui->imagemAnime02->clear();
                ui->imagemAnime02->setStyleSheet("background: black;");
            }
        }
        else{
            if(pix.load(cconfBase->vimagemStaple, "jpg"))
                ui->imagemAnime02->setPixmap(pix);
        }
    }
    else{
        ui->imagemAnime02->clear();
        ui->imagemAnime02->setStyleSheet("background: transparent;");
        ui->labelAnime02Titulo->clear();
        ui->labelAnime02Progresso->clear();
        ui->labelAnime02Nota->clear();
        ui->labelFundoAnime02Nota->setStyleSheet("background: transparent;");
        ui->labelFundoAnime02Progresso->setStyleSheet("background: transparent;");
        ui->labelFundoAnime02Titulo->setStyleSheet("background: transparent;");
        ui->labelFundoAnime02Lista->clear();
        ui->labelFundoAnime02Lista->setStyleSheet("background: transparent");
    }
    if(vlistaSelecionada.size() > 3+(12*(vpagina-1))){
        ui->imagemAnime03->setScaledContents(true);
        ui->labelFundoAnime03Nota->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime03Progresso->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime03Titulo->setStyleSheet("background: rgb(185,201,250);");
        if(vlistaSelecionada[3+(12*(vpagina-1))]->vnome.size() < 47)
            ui->labelAnime03Titulo->setStyleSheet(vfonteNormal);
        else if(vlistaSelecionada[3+(12*(vpagina-1))]->vnome.size() < 58)
            ui->labelAnime03Titulo->setStyleSheet(vtituloNormalMedio);
        else
            ui->labelAnime03Titulo->setStyleSheet(vtituloNormalPequeno);
        ui->labelAnime03Titulo->setText(vlistaSelecionada[3+(12*(vpagina-1))]->vnome);
        ui->labelAnime03Titulo->setAlignment(Qt::AlignCenter);
        ui->labelAnime03Titulo->setWordWrap(true);
        ui->labelAnime03Progresso->setAlignment(Qt::AlignCenter);
        if(!carquivos->fprocuraEpisodio(vlistaSelecionada[3+(12*(vpagina-1))]).isEmpty() && vtipoAtual == "anime")
            ui->labelAnime03Progresso->setStyleSheet(vfonteNormalAnimeEncontrado);
        else if(vlistaSelecionada[3+(12*(vpagina-1))]->vstatus.compare("Not Aired Yet", Qt::CaseInsensitive) == 0)
            ui->labelAnime03Progresso->setStyleSheet(vfonteNormalAnimeNaoComecou);
        else
            ui->labelAnime03Progresso->setStyleSheet(vfonteNormal);
        ui->labelAnime03Progresso->setText("Progresso: " + vlistaSelecionada[3+(12*(vpagina-1))]->vnumEpisodiosAssistidos +
                    "/" + vlistaSelecionada[3+(12*(vpagina-1))]->vnumEpisodiosTotais);
        ui->labelAnime03Nota->setAlignment(Qt::AlignCenter);
        ui->labelAnime03Nota->setStyleSheet(vfonteNormalNota);
        ui->labelAnime03Nota->setText("Nota: " + vlistaSelecionada[3+(12*(vpagina-1))]->vnotaMediaPessoal + "/10");
        if(vlistaAtual == "busca" || vlistaAtual.contains("season")){
            ui->labelFundoAnime03Lista->setStyleSheet("background: rgb(185, 202, 250); font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);");
            bool ok;
            //Checa se a lista é um número válido
            vlistaSelecionada[3+(12*(vpagina-1))]->vlista.toInt(&ok);
            if(ok)
                ui->labelFundoAnime03Lista->setText(vlistaSelecionada[3+(12*(vpagina-1))]->vSeasonSemAno);
            else
            ui->labelFundoAnime03Lista->setText(vlistaSelecionada[3+(12*(vpagina-1))]->vlista);
        }
        else{
            ui->labelFundoAnime03Lista->clear();
            ui->labelFundoAnime03Lista->setStyleSheet("background: transparent");
        }
        if(vimagemCarregada[vlistaSelecionada[3+(12*(vpagina-1))]->vid]){
            if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[3+(12*(vpagina-1))]->vid+".jpg", "jpg")){
                ui->imagemAnime03->setPixmap(pix);
            }
            else if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[3+(12*(vpagina-1))]->vid+".png", "png")){
                ui->imagemAnime03->setPixmap(pix);
            }
            else{
                ui->imagemAnime03->clear();
                ui->imagemAnime03->setStyleSheet("background: black;");
            }
        }
        else{
            if(pix.load(cconfBase->vimagemStaple, "jpg"))
                ui->imagemAnime03->setPixmap(pix);
        }
    }
    else{
        ui->imagemAnime03->clear();
        ui->imagemAnime03->setStyleSheet("background: transparent;");
        ui->labelAnime03Titulo->clear();
        ui->labelAnime03Progresso->clear();
        ui->labelAnime03Nota->clear();
        ui->labelFundoAnime03Nota->setStyleSheet("background: transparent;");
        ui->labelFundoAnime03Progresso->setStyleSheet("background: transparent;");
        ui->labelFundoAnime03Titulo->setStyleSheet("background: transparent;");
        ui->labelFundoAnime03Lista->clear();
        ui->labelFundoAnime03Lista->setStyleSheet("background: transparent");
    }
    if(vlistaSelecionada.size() > 4+(12*(vpagina-1))){
        ui->imagemAnime04->setScaledContents(true);
        ui->labelFundoAnime04Nota->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime04Progresso->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime04Titulo->setStyleSheet("background: rgb(185,201,250);");
        if(vlistaSelecionada[4+(12*(vpagina-1))]->vnome.size() < 47)
            ui->labelAnime04Titulo->setStyleSheet(vfonteNormal);
        else if(vlistaSelecionada[4+(12*(vpagina-1))]->vnome.size() < 58)
            ui->labelAnime04Titulo->setStyleSheet(vtituloNormalMedio);
        else
            ui->labelAnime04Titulo->setStyleSheet(vtituloNormalPequeno);
        ui->labelAnime04Titulo->setText(vlistaSelecionada[4+(12*(vpagina-1))]->vnome);
        ui->labelAnime04Titulo->setAlignment(Qt::AlignCenter);
        ui->labelAnime04Titulo->setWordWrap(true);
        ui->labelAnime04Progresso->setAlignment(Qt::AlignCenter);
        if(!carquivos->fprocuraEpisodio(vlistaSelecionada[4+(12*(vpagina-1))]).isEmpty() && vtipoAtual == "anime")
            ui->labelAnime04Progresso->setStyleSheet(vfonteNormalAnimeEncontrado);
        else if(vlistaSelecionada[4+(12*(vpagina-1))]->vstatus.compare("Not Aired Yet", Qt::CaseInsensitive) == 0)
            ui->labelAnime04Progresso->setStyleSheet(vfonteNormalAnimeNaoComecou);
        else
            ui->labelAnime04Progresso->setStyleSheet(vfonteNormal);
        ui->labelAnime04Progresso->setText("Progresso: " + vlistaSelecionada[4+(12*(vpagina-1))]->vnumEpisodiosAssistidos +
                    "/" + vlistaSelecionada[4+(12*(vpagina-1))]->vnumEpisodiosTotais);
        ui->labelAnime04Nota->setAlignment(Qt::AlignCenter);
        ui->labelAnime04Nota->setStyleSheet(vfonteNormalNota);
        ui->labelAnime04Nota->setText("Nota: " + vlistaSelecionada[4+(12*(vpagina-1))]->vnotaMediaPessoal + "/10");
        if(vlistaAtual == "busca" || vlistaAtual.contains("season")){
            ui->labelFundoAnime04Lista->setStyleSheet("background: rgb(185, 202, 250); font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);");
            bool ok;
            //Checa se a lista é um número válido
            vlistaSelecionada[4+(12*(vpagina-1))]->vlista.toInt(&ok);
            if(ok)
                ui->labelFundoAnime04Lista->setText(vlistaSelecionada[4+(12*(vpagina-1))]->vSeasonSemAno);
            else
            ui->labelFundoAnime04Lista->setText(vlistaSelecionada[4+(12*(vpagina-1))]->vlista);
        }
        else{
            ui->labelFundoAnime04Lista->clear();
            ui->labelFundoAnime04Lista->setStyleSheet("background: transparent");
        }
        if(vimagemCarregada[vlistaSelecionada[4+(12*(vpagina-1))]->vid]){
            if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[4+(12*(vpagina-1))]->vid+".jpg", "jpg")){
                ui->imagemAnime04->setPixmap(pix);
            }
            else if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[4+(12*(vpagina-1))]->vid+".png", "png")){
                ui->imagemAnime04->setPixmap(pix);
            }
            else{
                ui->imagemAnime04->clear();
                ui->imagemAnime04->setStyleSheet("background: black;");
            }
        }
        else{
            if(pix.load(cconfBase->vimagemStaple, "jpg"))
                ui->imagemAnime04->setPixmap(pix);
        }
    }
    else{
        ui->imagemAnime04->clear();
        ui->imagemAnime04->setStyleSheet("background: transparent;");
        ui->labelAnime04Titulo->clear();
        ui->labelAnime04Progresso->clear();
        ui->labelAnime04Nota->clear();
        ui->labelFundoAnime04Nota->setStyleSheet("background: transparent;");
        ui->labelFundoAnime04Progresso->setStyleSheet("background: transparent;");
        ui->labelFundoAnime04Titulo->setStyleSheet("background: transparent;");
        ui->labelFundoAnime04Lista->clear();
        ui->labelFundoAnime04Lista->setStyleSheet("background: transparent");
    }
    if(vlistaSelecionada.size() > 5+(12*(vpagina-1))){
        ui->imagemAnime05->setScaledContents(true);
        ui->labelFundoAnime05Nota->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime05Progresso->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime05Titulo->setStyleSheet("background: rgb(185,201,250);");
        if(vlistaSelecionada[5+(12*(vpagina-1))]->vnome.size() < 47)
            ui->labelAnime05Titulo->setStyleSheet(vfonteNormal);
        else if(vlistaSelecionada[5+(12*(vpagina-1))]->vnome.size() < 58)
            ui->labelAnime05Titulo->setStyleSheet(vtituloNormalMedio);
        else
            ui->labelAnime05Titulo->setStyleSheet(vtituloNormalPequeno);
        ui->labelAnime05Titulo->setText(vlistaSelecionada[5+(12*(vpagina-1))]->vnome);
        ui->labelAnime05Titulo->setAlignment(Qt::AlignCenter);
        ui->labelAnime05Titulo->setWordWrap(true);
        ui->labelAnime05Progresso->setAlignment(Qt::AlignCenter);
        ui->labelAnime05Progresso->setText("Progresso: " + vlistaSelecionada[5+(12*(vpagina-1))]->vnumEpisodiosAssistidos +
                    "/" + vlistaSelecionada[5+(12*(vpagina-1))]->vnumEpisodiosTotais);
        ui->labelAnime05Nota->setAlignment(Qt::AlignCenter);
        ui->labelAnime05Nota->setStyleSheet(vfonteNormalNota);
        if(!carquivos->fprocuraEpisodio(vlistaSelecionada[5+(12*(vpagina-1))]).isEmpty() && vtipoAtual == "anime")
            ui->labelAnime05Progresso->setStyleSheet(vfonteNormalAnimeEncontrado);
        else if(vlistaSelecionada[5+(12*(vpagina-1))]->vstatus.compare("Not Aired Yet", Qt::CaseInsensitive) == 0)
            ui->labelAnime05Progresso->setStyleSheet(vfonteNormalAnimeNaoComecou);
        else
            ui->labelAnime05Progresso->setStyleSheet(vfonteNormal);
        ui->labelAnime05Nota->setText("Nota: " + vlistaSelecionada[5+(12*(vpagina-1))]->vnotaMediaPessoal + "/10");
        if(vlistaAtual == "busca" || vlistaAtual.contains("season")){
            ui->labelFundoAnime05Lista->setStyleSheet("background: rgb(185, 202, 250); font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);");
            bool ok;
            //Checa se a lista é um número válido
            vlistaSelecionada[5+(12*(vpagina-1))]->vlista.toInt(&ok);
            if(ok)
                ui->labelFundoAnime05Lista->setText(vlistaSelecionada[5+(12*(vpagina-1))]->vSeasonSemAno);
            else
            ui->labelFundoAnime05Lista->setText(vlistaSelecionada[5+(12*(vpagina-1))]->vlista);
        }
        else{
            ui->labelFundoAnime05Lista->clear();
            ui->labelFundoAnime05Lista->setStyleSheet("background: transparent");
        }
        if(vimagemCarregada[vlistaSelecionada[5+(12*(vpagina-1))]->vid]){
            if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[5+(12*(vpagina-1))]->vid+".jpg", "jpg")){
                ui->imagemAnime05->setPixmap(pix);
            }
            else if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[5+(12*(vpagina-1))]->vid+".png", "png")){
                ui->imagemAnime05->setPixmap(pix);
            }
            else{
                ui->imagemAnime05->clear();
                ui->imagemAnime05->setStyleSheet("background: black;");
            }
        }
        else{
            if(pix.load(cconfBase->vimagemStaple, "jpg"))
                ui->imagemAnime05->setPixmap(pix);
        }
    }
    else{
        ui->imagemAnime05->clear();
        ui->imagemAnime05->setStyleSheet("background: transparent;");
        ui->labelAnime05Titulo->clear();
        ui->labelAnime05Progresso->clear();
        ui->labelAnime05Nota->clear();
        ui->labelFundoAnime05Nota->setStyleSheet("background: transparent;");
        ui->labelFundoAnime05Progresso->setStyleSheet("background: transparent;");
        ui->labelFundoAnime05Titulo->setStyleSheet("background: transparent;");
        ui->labelFundoAnime05Lista->clear();
        ui->labelFundoAnime05Lista->setStyleSheet("background: transparent");
    }
    if(vlistaSelecionada.size() > 6+(12*(vpagina-1))){
        ui->imagemAnime06->setScaledContents(true);
        ui->labelFundoAnime06Nota->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime06Progresso->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime06Titulo->setStyleSheet("background: rgb(185,201,250);");
        if(vlistaSelecionada[6+(12*(vpagina-1))]->vnome.size() < 47)
            ui->labelAnime06Titulo->setStyleSheet(vfonteNormal);
        else if(vlistaSelecionada[6+(12*(vpagina-1))]->vnome.size() < 58)
            ui->labelAnime06Titulo->setStyleSheet(vtituloNormalMedio);
        else
            ui->labelAnime06Titulo->setStyleSheet(vtituloNormalPequeno);
        ui->labelAnime06Titulo->setText(vlistaSelecionada[6+(12*(vpagina-1))]->vnome);
        ui->labelAnime06Titulo->setAlignment(Qt::AlignCenter);
        ui->labelAnime06Titulo->setWordWrap(true);
        ui->labelAnime06Progresso->setAlignment(Qt::AlignCenter);
        ui->labelAnime06Progresso->setText("Progresso: " + vlistaSelecionada[6+(12*(vpagina-1))]->vnumEpisodiosAssistidos +
                    "/" + vlistaSelecionada[6+(12*(vpagina-1))]->vnumEpisodiosTotais);
        ui->labelAnime06Nota->setAlignment(Qt::AlignCenter);
        ui->labelAnime06Nota->setStyleSheet(vfonteNormalNota);
        if(!carquivos->fprocuraEpisodio(vlistaSelecionada[6+(12*(vpagina-1))]).isEmpty() && vtipoAtual == "anime")
            ui->labelAnime06Progresso->setStyleSheet(vfonteNormalAnimeEncontrado);
        else if(vlistaSelecionada[6+(12*(vpagina-1))]->vstatus.compare("Not Aired Yet", Qt::CaseInsensitive) == 0)
            ui->labelAnime06Progresso->setStyleSheet(vfonteNormalAnimeNaoComecou);
        else
            ui->labelAnime06Progresso->setStyleSheet(vfonteNormal);
        ui->labelAnime06Nota->setText("Nota: " + vlistaSelecionada[6+(12*(vpagina-1))]->vnotaMediaPessoal + "/10");
        if(vlistaAtual == "busca" || vlistaAtual.contains("season")){
            ui->labelFundoAnime06Lista->setStyleSheet("background: rgb(185, 202, 250); font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);");
            bool ok;
            //Checa se a lista é um número válido
            vlistaSelecionada[6+(12*(vpagina-1))]->vlista.toInt(&ok);
            if(ok)
                ui->labelFundoAnime06Lista->setText(vlistaSelecionada[6+(12*(vpagina-1))]->vSeasonSemAno);
            else
                ui->labelFundoAnime06Lista->setText(vlistaSelecionada[6+(12*(vpagina-1))]->vlista);
        }
        else{
            ui->labelFundoAnime06Lista->clear();
            ui->labelFundoAnime06Lista->setStyleSheet("background: transparent");
        }
        if(vimagemCarregada[vlistaSelecionada[6+(12*(vpagina-1))]->vid]){
            if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[6+(12*(vpagina-1))]->vid+".jpg", "jpg")){
                ui->imagemAnime06->setPixmap(pix);
            }
            else if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[6+(12*(vpagina-1))]->vid+".png", "png")){
                ui->imagemAnime06->setPixmap(pix);
            }
            else{
                ui->imagemAnime06->clear();
                ui->imagemAnime06->setStyleSheet("background: black;");
            }
        }
        else{
            if(pix.load(cconfBase->vimagemStaple, "jpg"))
                ui->imagemAnime06->setPixmap(pix);
        }
    }
    else{
        ui->imagemAnime06->clear();
        ui->imagemAnime06->setStyleSheet("background: transparent;");
        ui->labelAnime06Titulo->clear();
        ui->labelAnime06Progresso->clear();
        ui->labelAnime06Nota->clear();
        ui->labelFundoAnime06Nota->setStyleSheet("background: transparent;");
        ui->labelFundoAnime06Progresso->setStyleSheet("background: transparent;");
        ui->labelFundoAnime06Titulo->setStyleSheet("background: transparent;");
        ui->labelFundoAnime06Lista->clear();
        ui->labelFundoAnime06Lista->setStyleSheet("background: transparent");
    }
    if(vlistaSelecionada.size() > 7+(12*(vpagina-1))){
        ui->imagemAnime07->setScaledContents(true);
        ui->labelFundoAnime07Nota->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime07Progresso->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime07Titulo->setStyleSheet("background: rgb(185,201,250);");
        if(vlistaSelecionada[7+(12*(vpagina-1))]->vnome.size() < 47)
            ui->labelAnime07Titulo->setStyleSheet(vfonteNormal);
        else if(vlistaSelecionada[7+(12*(vpagina-1))]->vnome.size() < 58)
            ui->labelAnime07Titulo->setStyleSheet(vtituloNormalMedio);
        else
            ui->labelAnime07Titulo->setStyleSheet(vtituloNormalPequeno);
        ui->labelAnime07Titulo->setText(vlistaSelecionada[7+(12*(vpagina-1))]->vnome);
        ui->labelAnime07Titulo->setAlignment(Qt::AlignCenter);
        ui->labelAnime07Titulo->setWordWrap(true);
        ui->labelAnime07Progresso->setAlignment(Qt::AlignCenter);
        if(!carquivos->fprocuraEpisodio(vlistaSelecionada[7+(12*(vpagina-1))]).isEmpty() && vtipoAtual == "anime")
            ui->labelAnime07Progresso->setStyleSheet(vfonteNormalAnimeEncontrado);
        else if(vlistaSelecionada[7+(12*(vpagina-1))]->vstatus.compare("Not Aired Yet", Qt::CaseInsensitive) == 0)
            ui->labelAnime07Progresso->setStyleSheet(vfonteNormalAnimeNaoComecou);
        else
            ui->labelAnime07Progresso->setStyleSheet(vfonteNormal);
        ui->labelAnime07Progresso->setText("Progresso: " + vlistaSelecionada[7+(12*(vpagina-1))]->vnumEpisodiosAssistidos +
                    "/" + vlistaSelecionada[7+(12*(vpagina-1))]->vnumEpisodiosTotais);
        ui->labelAnime07Nota->setAlignment(Qt::AlignCenter);
        ui->labelAnime07Nota->setStyleSheet(vfonteNormalNota);
        ui->labelAnime07Nota->setText("Nota: " + vlistaSelecionada[7+(12*(vpagina-1))]->vnotaMediaPessoal + "/10");
        if(vlistaAtual == "busca" || vlistaAtual.contains("season")){
            ui->labelFundoAnime07Lista->setStyleSheet("background: rgb(185, 202, 250); font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);");
            bool ok;
            //Checa se a lista é um número válido
            vlistaSelecionada[7+(12*(vpagina-1))]->vlista.toInt(&ok);
            if(ok)
                ui->labelFundoAnime07Lista->setText(vlistaSelecionada[7+(12*(vpagina-1))]->vSeasonSemAno);
            else
            ui->labelFundoAnime07Lista->setText(vlistaSelecionada[7+(12*(vpagina-1))]->vlista);
        }
        else{
            ui->labelFundoAnime07Lista->clear();
            ui->labelFundoAnime07Lista->setStyleSheet("background: transparent");
        }
        if(vimagemCarregada[vlistaSelecionada[7+(12*(vpagina-1))]->vid]){
            if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[7+(12*(vpagina-1))]->vid+".jpg", "jpg")){
                ui->imagemAnime07->setPixmap(pix);
            }
            else if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[7+(12*(vpagina-1))]->vid+".png", "png")){
                ui->imagemAnime07->setPixmap(pix);
            }
            else{
                ui->imagemAnime07->clear();
                ui->imagemAnime07->setStyleSheet("background: black;");
            }
        }
        else{
            if(pix.load(cconfBase->vimagemStaple, "jpg"))
                ui->imagemAnime07->setPixmap(pix);
        }
    }
    else{
        ui->imagemAnime07->clear();
        ui->imagemAnime07->setStyleSheet("background: transparent;");
        ui->labelAnime07Titulo->clear();
        ui->labelAnime07Progresso->clear();
        ui->labelAnime07Nota->clear();
        ui->labelFundoAnime07Nota->setStyleSheet("background: transparent;");
        ui->labelFundoAnime07Progresso->setStyleSheet("background: transparent;");
        ui->labelFundoAnime07Titulo->setStyleSheet("background: transparent;");
        ui->labelFundoAnime07Lista->clear();
        ui->labelFundoAnime07Lista->setStyleSheet("background: transparent");
    }
    if(vlistaSelecionada.size() > 8+(12*(vpagina-1))){
        ui->imagemAnime08->setScaledContents(true);
        ui->labelFundoAnime08Nota->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime08Progresso->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime08Titulo->setStyleSheet("background: rgb(185,201,250);");
        if(vlistaSelecionada[8+(12*(vpagina-1))]->vnome.size() < 47)
            ui->labelAnime08Titulo->setStyleSheet(vfonteNormal);
        else if(vlistaSelecionada[8+(12*(vpagina-1))]->vnome.size() < 58)
            ui->labelAnime08Titulo->setStyleSheet(vtituloNormalMedio);
        else
            ui->labelAnime08Titulo->setStyleSheet(vtituloNormalPequeno);
        ui->labelAnime08Titulo->setText(vlistaSelecionada[8+(12*(vpagina-1))]->vnome);
        ui->labelAnime08Titulo->setAlignment(Qt::AlignCenter);
        ui->labelAnime08Titulo->setWordWrap(true);
        ui->labelAnime08Progresso->setAlignment(Qt::AlignCenter);
        if(!carquivos->fprocuraEpisodio(vlistaSelecionada[8+(12*(vpagina-1))]).isEmpty() && vtipoAtual == "anime")
            ui->labelAnime08Progresso->setStyleSheet(vfonteNormalAnimeEncontrado);
        else if(vlistaSelecionada[8+(12*(vpagina-1))]->vstatus.compare("Not Aired Yet", Qt::CaseInsensitive) == 0)
            ui->labelAnime08Progresso->setStyleSheet(vfonteNormalAnimeNaoComecou);
        else
            ui->labelAnime08Progresso->setStyleSheet(vfonteNormal);
        ui->labelAnime08Progresso->setText("Progresso: " + vlistaSelecionada[8+(12*(vpagina-1))]->vnumEpisodiosAssistidos +
                    "/" + vlistaSelecionada[8+(12*(vpagina-1))]->vnumEpisodiosTotais);
        ui->labelAnime08Nota->setAlignment(Qt::AlignCenter);
        ui->labelAnime08Nota->setStyleSheet(vfonteNormalNota);
        ui->labelAnime08Nota->setText("Nota: " + vlistaSelecionada[8+(12*(vpagina-1))]->vnotaMediaPessoal + "/10");
        if(vlistaAtual == "busca" || vlistaAtual.contains("season")){
            ui->labelFundoAnime08Lista->setStyleSheet("background: rgb(185, 202, 250); font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);");
            bool ok;
            //Checa se a lista é um número válido
            vlistaSelecionada[8+(12*(vpagina-1))]->vlista.toInt(&ok);
            if(ok)
                ui->labelFundoAnime08Lista->setText(vlistaSelecionada[8+(12*(vpagina-1))]->vSeasonSemAno);
            else
            ui->labelFundoAnime08Lista->setText(vlistaSelecionada[8+(12*(vpagina-1))]->vlista);
        }
        else{
            ui->labelFundoAnime08Lista->clear();
            ui->labelFundoAnime08Lista->setStyleSheet("background: transparent");
        }
        if(vimagemCarregada[vlistaSelecionada[8+(12*(vpagina-1))]->vid]){
            if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[8+(12*(vpagina-1))]->vid+".jpg", "jpg")){
                ui->imagemAnime08->setPixmap(pix);
            }
            else if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[8+(12*(vpagina-1))]->vid+".png", "png")){
                ui->imagemAnime08->setPixmap(pix);
            }
            else{
                ui->imagemAnime08->clear();
                ui->imagemAnime08->setStyleSheet("background: black;");
            }
        }
        else{
            if(pix.load(cconfBase->vimagemStaple, "jpg"))
                ui->imagemAnime08->setPixmap(pix);
        }
    }
    else{
        ui->imagemAnime08->clear();
        ui->imagemAnime08->setStyleSheet("background: transparent;");
        ui->labelAnime08Titulo->clear();
        ui->labelAnime08Progresso->clear();
        ui->labelAnime08Nota->clear();
        ui->labelFundoAnime08Nota->setStyleSheet("background: transparent;");
        ui->labelFundoAnime08Progresso->setStyleSheet("background: transparent;");
        ui->labelFundoAnime08Titulo->setStyleSheet("background: transparent;");
        ui->labelFundoAnime08Lista->clear();
        ui->labelFundoAnime08Lista->setStyleSheet("background: transparent");
    }
    if(vlistaSelecionada.size() > 9+(12*(vpagina-1))){
        ui->imagemAnime09->setScaledContents(true);
        ui->labelFundoAnime09Nota->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime09Progresso->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime09Titulo->setStyleSheet("background: rgb(185,201,250);");
        if(vlistaSelecionada[9+(12*(vpagina-1))]->vnome.size() < 47)
            ui->labelAnime09Titulo->setStyleSheet(vfonteNormal);
        else if(vlistaSelecionada[9+(12*(vpagina-1))]->vnome.size() < 58)
            ui->labelAnime09Titulo->setStyleSheet(vtituloNormalMedio);
        else
            ui->labelAnime09Titulo->setStyleSheet(vtituloNormalPequeno);
        ui->labelAnime09Titulo->setText(vlistaSelecionada[9+(12*(vpagina-1))]->vnome);
        ui->labelAnime09Titulo->setAlignment(Qt::AlignCenter);
        ui->labelAnime09Titulo->setWordWrap(true);
        ui->labelAnime09Progresso->setAlignment(Qt::AlignCenter);
        if(!carquivos->fprocuraEpisodio(vlistaSelecionada[9+(12*(vpagina-1))]).isEmpty() && vtipoAtual == "anime")
            ui->labelAnime09Progresso->setStyleSheet(vfonteNormalAnimeEncontrado);
        else if(vlistaSelecionada[9+(12*(vpagina-1))]->vstatus.compare("Not Aired Yet", Qt::CaseInsensitive) == 0)
            ui->labelAnime09Progresso->setStyleSheet(vfonteNormalAnimeNaoComecou);
        else
            ui->labelAnime09Progresso->setStyleSheet(vfonteNormal);
        ui->labelAnime09Progresso->setText("Progresso: " + vlistaSelecionada[9+(12*(vpagina-1))]->vnumEpisodiosAssistidos +
                    "/" + vlistaSelecionada[9+(12*(vpagina-1))]->vnumEpisodiosTotais);
        ui->labelAnime09Nota->setAlignment(Qt::AlignCenter);
        ui->labelAnime09Nota->setStyleSheet(vfonteNormalNota);
        ui->labelAnime09Nota->setText("Nota: " + vlistaSelecionada[9+(12*(vpagina-1))]->vnotaMediaPessoal + "/10");
        if(vlistaAtual == "busca" || vlistaAtual.contains("season")){
            ui->labelFundoAnime09Lista->setStyleSheet("background: rgb(185, 202, 250); font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);");
            bool ok;
            //Checa se a lista é um número válido
            vlistaSelecionada[9+(12*(vpagina-1))]->vlista.toInt(&ok);
            if(ok)
                ui->labelFundoAnime09Lista->setText(vlistaSelecionada[9+(12*(vpagina-1))]->vSeasonSemAno);
            else
            ui->labelFundoAnime09Lista->setText(vlistaSelecionada[9+(12*(vpagina-1))]->vlista);
        }
        else{
            ui->labelFundoAnime09Lista->clear();
            ui->labelFundoAnime09Lista->setStyleSheet("background: transparent");
        }
        if(vimagemCarregada[vlistaSelecionada[9+(12*(vpagina-1))]->vid]){
            if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[9+(12*(vpagina-1))]->vid+".jpg", "jpg")){
                ui->imagemAnime09->setPixmap(pix);
            }
            else if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[9+(12*(vpagina-1))]->vid+".png", "png")){
                ui->imagemAnime09->setPixmap(pix);
            }
            else{
                ui->imagemAnime09->clear();
                ui->imagemAnime09->setStyleSheet("background: black;");
            }
        }
        else{
            if(pix.load(cconfBase->vimagemStaple, "jpg"))
                ui->imagemAnime09->setPixmap(pix);
        }
    }
    else{
        ui->imagemAnime09->clear();
        ui->imagemAnime09->setStyleSheet("background: transparent;");
        ui->labelAnime09Titulo->clear();
        ui->labelAnime09Progresso->clear();
        ui->labelAnime09Nota->clear();
        ui->labelFundoAnime09Nota->setStyleSheet("background: transparent;");
        ui->labelFundoAnime09Progresso->setStyleSheet("background: transparent;");
        ui->labelFundoAnime09Titulo->setStyleSheet("background: transparent;");
        ui->labelFundoAnime09Lista->clear();
        ui->labelFundoAnime09Lista->setStyleSheet("background: transparent");
    }
    if(vlistaSelecionada.size() > 10+(12*(vpagina-1))){
        ui->imagemAnime10->setScaledContents(true);
        ui->labelFundoAnime10Nota->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime10Progresso->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime10Titulo->setStyleSheet("background: rgb(185,201,250);");
        if(vlistaSelecionada[10+(12*(vpagina-1))]->vnome.size() < 47)
            ui->labelAnime10Titulo->setStyleSheet(vfonteNormal);
        else if(vlistaSelecionada[10+(12*(vpagina-1))]->vnome.size() < 58)
            ui->labelAnime10Titulo->setStyleSheet(vtituloNormalMedio);
        else
            ui->labelAnime10Titulo->setStyleSheet(vtituloNormalPequeno);
        ui->labelAnime10Titulo->setText(vlistaSelecionada[10+(12*(vpagina-1))]->vnome);
        ui->labelAnime10Titulo->setAlignment(Qt::AlignCenter);
        ui->labelAnime10Titulo->setWordWrap(true);
        ui->labelAnime10Progresso->setAlignment(Qt::AlignCenter);
        if(!carquivos->fprocuraEpisodio(vlistaSelecionada[10+(12*(vpagina-1))]).isEmpty() && vtipoAtual == "anime")
            ui->labelAnime10Progresso->setStyleSheet(vfonteNormalAnimeEncontrado);
        else if(vlistaSelecionada[10+(12*(vpagina-1))]->vstatus.compare("Not Aired Yet", Qt::CaseInsensitive) == 0)
            ui->labelAnime10Progresso->setStyleSheet(vfonteNormalAnimeNaoComecou);
        else
            ui->labelAnime10Progresso->setStyleSheet(vfonteNormal);
        ui->labelAnime10Progresso->setText("Progresso: " + vlistaSelecionada[10+(12*(vpagina-1))]->vnumEpisodiosAssistidos +
                    "/" + vlistaSelecionada[10+(12*(vpagina-1))]->vnumEpisodiosTotais);
        ui->labelAnime10Nota->setAlignment(Qt::AlignCenter);
        ui->labelAnime10Nota->setStyleSheet(vfonteNormalNota);
        ui->labelAnime10Nota->setText("Nota: " + vlistaSelecionada[10+(12*(vpagina-1))]->vnotaMediaPessoal + "/10");
        if(vlistaAtual == "busca" || vlistaAtual.contains("season")){
            ui->labelFundoAnime10Lista->setStyleSheet("background: rgb(185, 202, 250); font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);");
            bool ok;
            //Checa se a lista é um número válido
            vlistaSelecionada[10+(12*(vpagina-1))]->vlista.toInt(&ok);
            if(ok)
                ui->labelFundoAnime10Lista->setText(vlistaSelecionada[10+(12*(vpagina-1))]->vSeasonSemAno);
            else
                ui->labelFundoAnime10Lista->setText(vlistaSelecionada[10+(12*(vpagina-1))]->vlista);
        }
        else{
            ui->labelFundoAnime10Lista->clear();
            ui->labelFundoAnime10Lista->setStyleSheet("background: transparent");
        }
        if(vimagemCarregada[vlistaSelecionada[10+(12*(vpagina-1))]->vid]){
            if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[10+(12*(vpagina-1))]->vid+".jpg", "jpg")){
                ui->imagemAnime10->setPixmap(pix);
            }
            else if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[10+(12*(vpagina-1))]->vid+".png", "png")){
                ui->imagemAnime10->setPixmap(pix);
            }
            else{
                ui->imagemAnime10->clear();
                ui->imagemAnime10->setStyleSheet("background: black;");
            }
        }
        else{
            if(pix.load(cconfBase->vimagemStaple, "jpg"))
                ui->imagemAnime10->setPixmap(pix);
        }
    }
    else{
        ui->imagemAnime10->clear();
        ui->imagemAnime10->setStyleSheet("background: transparent;");
        ui->labelAnime10Titulo->clear();
        ui->labelAnime10Progresso->clear();
        ui->labelAnime10Nota->clear();
        ui->labelFundoAnime10Nota->setStyleSheet("background: transparent;");
        ui->labelFundoAnime10Progresso->setStyleSheet("background: transparent;");
        ui->labelFundoAnime10Titulo->setStyleSheet("background: transparent;");
        ui->labelFundoAnime10Lista->clear();
        ui->labelFundoAnime10Lista->setStyleSheet("background: transparent");
    }
    if(vlistaSelecionada.size() > 11+(12*(vpagina-1))){
        ui->imagemAnime11->setScaledContents(true);
        ui->labelFundoAnime11Nota->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime11Progresso->setStyleSheet("background: rgb(185,201,250);");
        ui->labelFundoAnime11Titulo->setStyleSheet("background: rgb(185,201,250);");
        if(vlistaSelecionada[11+(12*(vpagina-1))]->vnome.size() < 47)
            ui->labelAnime11Titulo->setStyleSheet(vfonteNormal);
        else if(vlistaSelecionada[11+(12*(vpagina-1))]->vnome.size() < 58)
            ui->labelAnime11Titulo->setStyleSheet(vtituloNormalMedio);
        else
            ui->labelAnime11Titulo->setStyleSheet(vtituloNormalPequeno);
        ui->labelAnime11Titulo->setText(vlistaSelecionada[11+(12*(vpagina-1))]->vnome);
        ui->labelAnime11Titulo->setAlignment(Qt::AlignCenter);
        ui->labelAnime11Titulo->setWordWrap(true);
        ui->labelAnime11Progresso->setAlignment(Qt::AlignCenter);
        if(!carquivos->fprocuraEpisodio(vlistaSelecionada[11+(12*(vpagina-1))]).isEmpty() && vtipoAtual == "anime")
            ui->labelAnime11Progresso->setStyleSheet(vfonteNormalAnimeEncontrado);
        else if(vlistaSelecionada[11+(12*(vpagina-1))]->vstatus.compare("Not Aired Yet", Qt::CaseInsensitive) == 0)
            ui->labelAnime11Progresso->setStyleSheet(vfonteNormalAnimeNaoComecou);
        else
            ui->labelAnime11Progresso->setStyleSheet(vfonteNormal);
        ui->labelAnime11Progresso->setText("Progresso: " + vlistaSelecionada[11+(12*(vpagina-1))]->vnumEpisodiosAssistidos +
                    "/" + vlistaSelecionada[11+(12*(vpagina-1))]->vnumEpisodiosTotais);
        ui->labelAnime11Nota->setAlignment(Qt::AlignCenter);
        ui->labelAnime11Nota->setStyleSheet(vfonteNormalNota);
        ui->labelAnime11Nota->setText("Nota: " + vlistaSelecionada[11+(12*(vpagina-1))]->vnotaMediaPessoal + "/10");
        if(vlistaAtual == "busca" || vlistaAtual.contains("season")){
            ui->labelFundoAnime11Lista->setStyleSheet("background: rgb(185, 202, 250); font: 75 8pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);");
            bool ok;
            //Checa se a lista é um número válido
            vlistaSelecionada[11+(12*(vpagina-1))]->vlista.toInt(&ok);
            if(ok)
                ui->labelFundoAnime11Lista->setText(vlistaSelecionada[11+(12*(vpagina-1))]->vSeasonSemAno);
            else
                ui->labelFundoAnime11Lista->setText(vlistaSelecionada[11+(12*(vpagina-1))]->vlista);
        }
        else{
            ui->labelFundoAnime11Lista->clear();
            ui->labelFundoAnime11Lista->setStyleSheet("background: transparent");
        }
        if(vimagemCarregada[vlistaSelecionada[11+(12*(vpagina-1))]->vid]){
            if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[11+(12*(vpagina-1))]->vid+".jpg", "jpg")){
                ui->imagemAnime11->setPixmap(pix);
            }
            else if(pix.load(cconfBase->vdiretorioImagensMedio+vlistaSelecionada[11+(12*(vpagina-1))]->vid+".png", "png")){
                ui->imagemAnime11->setPixmap(pix);
            }
            else{
                ui->imagemAnime11->clear();
                ui->imagemAnime11->setStyleSheet("background: black;");
            }
        }
        else{
            if(pix.load(cconfBase->vimagemStaple, "jpg"))
                ui->imagemAnime11->setPixmap(pix);
        }
    }
    else{
        ui->imagemAnime11->clear();
        ui->imagemAnime11->setStyleSheet("background: transparent;");
        ui->labelAnime11Titulo->clear();
        ui->labelAnime11Progresso->clear();
        ui->labelAnime11Nota->clear();
        ui->labelFundoAnime11Nota->setStyleSheet("background: transparent;");
        ui->labelFundoAnime11Progresso->setStyleSheet("background: transparent;");
        ui->labelFundoAnime11Titulo->setStyleSheet("background: transparent;");
        ui->labelFundoAnime11Lista->clear();
        ui->labelFundoAnime11Lista->setStyleSheet("background: transparent");
    }
}

bool MainWindow::fcarregaImagensBackground(const QString &lista){
    if(lista.compare("small", Qt::CaseInsensitive) == 0){
        vdownloadImagensPequenas = true;
        cfiledownloader->fresetCounters();
        cfiledownloader->fdownloadMedio();
    }
    else if(lista.compare("medium", Qt::CaseInsensitive) == 0){
        vdownloadImagensMedias = true;
        if(!vusarImagensBaixaQualidade){
            cfiledownloader->fresetCounters();
            cfiledownloader->fdownloadGrande();
        }
        else
            cfiledownloader->fdownloadAvatarUsuario(canilist->fretornaAvatar());
    }
    else if(lista.compare("big", Qt::CaseInsensitive) == 0){
        vdownloadImagensGrandes = true;
        cfiledownloader->fdownloadAvatarUsuario(canilist->fretornaAvatar());
    }
    else if(lista.compare("ano", Qt::CaseInsensitive) == 0){
        vdownloadImagensAno = true;
    }
    if(!vcarregaImagens.isRunning() && vdownloadImagensAcabou)
        vcarregaImagens = QtConcurrent::run(this, &MainWindow::fcarregaImagensSelecionadasBackground);
    return true;
}

//Dá load em todas as imagens disponíveis no computador para não ter que carregar elas depois
bool MainWindow::fcarregaImagensSelecionadasBackground(){
    QPixmap lpix;
    QString file;
    vdownloadImagensAcabou = false;
    QMutexLocker llocker(&vmutex);
    if(vrefreshAcontecendo)
        return false;
    if(vdownloadImagensPequenas){
        file = dirPequeno;
        foreach(QString id, vimagemBaixada.keys()){
            if(vrefreshAcontecendo)
                return false;
            if(!vimagemCarregada[id] && vimagemBaixada[id]){
                if(QFile::exists(file+id+".jpg")){
                    if(!lpix.paintingActive() && lpix.load(file+id+".jpg", "jpg", Qt::ColorOnly)){
                        ui->labelImagemBackground->setPixmap(lpix);
                        vimagemCarregada[id] = true;
                    }
                }
                else if(QFile::exists(file+id+".png")){
                    if(!lpix.paintingActive() && lpix.load(file+id+".png", "png", Qt::ColorOnly)){
                        ui->labelImagemBackground->setPixmap(lpix);
                        vimagemCarregada[id] = true;
                    }
                }
            }
            if(!lpix.paintingActive()){
                lpix = QPixmap();
                ui->labelImagemBackground->setPixmap(lpix);
            }
        }
        vdownloadImagensPequenas = false;
    }
    if(vdownloadImagensMedias){
        file = dirMedio;
        vprogressoLoadingBar = 0;
        vtotalAnimesLoadingBar = vimagemBaixada.size();
        foreach(QString id, vimagemBaixada.keys()){
            if(vrefreshAcontecendo)
                return false;
            if(!vusarImagensBaixaQualidade){
                if(!vimagemCarregada[id] && vimagemBaixada[id]){
                    if(QFile::exists(file+id+".jpg")){
                        if(lpix.load(file+id+".jpg", "jpg", Qt::ColorOnly)){
                            ui->labelImagemBackground->setPixmap(lpix);
                            vimagemCarregada[id] = true;
                        }
                    }
                    else if(QFile::exists(file+id+".png")){
                        if(lpix.load(file+id+".png", "png", Qt::ColorOnly)){
                            ui->labelImagemBackground->setPixmap(lpix);
                            vimagemCarregada[id] = true;
                        }
                    }
                    if(vprogressoLoadingBar != vtotalAnimesLoadingBar)
                        vprogressoLoadingBar++;
                    ui->labelLoading->setText("Loaded " + QString::number(vprogressoLoadingBar) + " of " +
                                              QString::number(vtotalAnimesLoadingBar) + " images");
                }
            }
            else{
                if(!vimagemCarregadaGrande[id] && vimagemBaixadaGrande[id]){
                    if(QFile::exists(file+id+".jpg")){
                        if(!lpix.paintingActive() && lpix.load(file+id+".jpg", "jpg", Qt::ColorOnly)){
                            ui->labelImagemBackground->setPixmap(lpix);
                            vimagemCarregadaGrande[id] = true;
                        }
                    }
                    else if(QFile::exists(file+id+".png")){
                        if( !lpix.paintingActive() && lpix.load(file+id+".png", "png", Qt::ColorOnly)){
                            ui->labelImagemBackground->setPixmap(lpix);
                            vimagemCarregadaGrande[id] = true;
                        }
                    }
                }
            }
            if(!lpix.paintingActive()){
                lpix = QPixmap();
                ui->labelImagemBackground->setPixmap(lpix);
            }
        }
        vdownloadImagensMedias = false;
    }
    if(vdownloadImagensGrandes){
        file = dirGrande;
        foreach(QString id, vimagemBaixadaGrande.keys()){
            if(vrefreshAcontecendo)
                return false;
            if(!vimagemCarregada[id] && vimagemBaixada[id]){
                if(QFile::exists(file+id+".jpg")){
                    if(!lpix.paintingActive() && lpix.load(file+id+".jpg", "jpg", Qt::ColorOnly)){
                        ui->labelImagemBackgroundGrande->setPixmap(lpix);
                        vimagemCarregadaGrande[id] = true;
                    }
                }
                else if(QFile::exists(file+id+".png")){
                    if(!lpix.paintingActive() && lpix.load(file+id+".png", "png", Qt::ColorOnly)){
                        ui->labelImagemBackgroundGrande->setPixmap(lpix);
                        vimagemCarregadaGrande[id] = true;
                    }
                }
            }
            if(!lpix.paintingActive()){
                lpix = QPixmap();
                ui->labelImagemBackgroundGrande->setPixmap(lpix);
            }
        }
        vdownloadImagensGrandes = false;
    }
    if(vdownloadImagensAno){
        vprogressoLoadingBar = 0;
        file = dirMedio;
        foreach(QString id, vimagemBaixada.keys()){
            if(vrefreshAcontecendo)
                return false;
            if(!vimagemCarregada[id] && vimagemBaixada[id]){
                if(QFile::exists(file+id+".jpg")){
                    if(!lpix.paintingActive() && lpix.load(file+id+".jpg", "jpg", Qt::ColorOnly)){
                        ui->labelImagemBackground->setPixmap(lpix);
                        vimagemCarregada[id] = true;
                    }
                }
                else if(QFile::exists(file+id+".png")){
                    if(!lpix.paintingActive() && lpix.load(file+id+".png", "png", Qt::ColorOnly)){
                        ui->labelImagemBackground->setPixmap(lpix);
                        vimagemCarregada[id] = true;
                    }
                }
                if(vprogressoLoadingBar != vtotalAnimesLoadingBar)
                    vprogressoLoadingBar++;
                ui->labelLoading->setText("Loaded " + QString::number(vprogressoLoadingBar) + " of " +
                                          QString::number(vtotalAnimesLoadingBar) + " images");
            }
            if(!lpix.paintingActive()){
                lpix = QPixmap();
                ui->labelImagemBackground->setPixmap(lpix);
            }
        }
        vbaixandoImagensAno = false;
        vdownloadImagensAno = false;
    }
    ui->labelImagemBackgroundGrande->clear();
    ui->labelImagemBackground->clear();
    vdownloadImagensAcabou = true;
    qDebug() << "Finished loading all pictures";
    emit sterminouCarregarImagens();
    return true;
}

void MainWindow::on_botaoAnime00_clicked()
{
    qDebug() << "oi";
    if(vlistaSelecionada.size() > 0+(12*(vpagina-1))){
        vanimeSelecionado = 0+(12*(vpagina-1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoAnime01_clicked()
{
    if(vlistaSelecionada.size() > 1+(12*(vpagina-1))){
        vanimeSelecionado = 1+(12*(vpagina-1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoAnime02_clicked()
{
    if(vlistaSelecionada.size() > 2+(12*(vpagina-1))){
        vanimeSelecionado = 2+(12*(vpagina-1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoAnime03_clicked()
{
    if(vlistaSelecionada.size() > 3+(12*(vpagina-1))){
        vanimeSelecionado = 3+(12*(vpagina-1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoAnime04_clicked()
{
    if(vlistaSelecionada.size() > 4+(12*(vpagina-1))){
        vanimeSelecionado = 4+(12*(vpagina-1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoAnime05_clicked()
{
    if(vlistaSelecionada.size() > 5+(12*(vpagina-1))){
        vanimeSelecionado = 5+(12*(vpagina-1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoAnime06_clicked()
{
    if(vlistaSelecionada.size() > 6+(12*(vpagina-1))){
        vanimeSelecionado = 6+(12*(vpagina-1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoAnime07_clicked()
{
    if(vlistaSelecionada.size() > 7+(12*(vpagina-1))){
        vanimeSelecionado = 7+(12*(vpagina-1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoAnime08_clicked()
{
    if(vlistaSelecionada.size() > 8+(12*(vpagina-1))){
        vanimeSelecionado = 8+(12*(vpagina-1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoAnime09_clicked()
{
    if(vlistaSelecionada.size() > 9+(12*(vpagina-1))){
        vanimeSelecionado = 9+(12*(vpagina-1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoAnime10_clicked()
{
    if(vlistaSelecionada.size() > 10+(12*(vpagina-1))){
        vanimeSelecionado = 10+(12*(vpagina-1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoAnime11_clicked()
{
    if(vlistaSelecionada.size() > 11+(12*(vpagina-1))){
        vanimeSelecionado = 11+(12*(vpagina-1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoProximaPagina_clicked()
{
    qDebug() << "Trying to go to page " << vpagina+1 << " from " << vpagina;
    if(vlistaSelecionada.size() > 12+(12*(vpagina-1))){
        vpagina++;
        QString llistaAtual;
        if(vlistaAtual == "watching")
            llistaAtual = "Watching";
        else if(vlistaAtual.contains("reading"))
            llistaAtual = "Reading";
        else if(vlistaAtual.contains("completed"))
            llistaAtual = "Completed";
        else if(vlistaAtual.contains("onhold"))
            llistaAtual = "On Hold";
        else if(vlistaAtual.contains("dropped"))
            llistaAtual = "Dropped";
        else if(vlistaAtual.contains("plantowatch"))
            llistaAtual = "Plan to Watch";
        else if(vlistaAtual.contains("busca"))
            llistaAtual = "Busca";
        ui->NumPagina->setText(llistaAtual + " - " + QString::number(vlistaSelecionada.size()) +
                               " animes in the list - Page "+QString::number(vpagina)+"/"+
                               QString::number(((vlistaSelecionada.size()-1)/12)+1));
        fcarregaImagensLista();
    }
}

void MainWindow::on_botaoPaginaAnterior_clicked()
{
    qDebug() << "Trying to go to page " << vpagina-1 << " from " << vpagina;
    if(vpagina > 1){
        vpagina--;
        QString llistaAtual;
        if(vlistaAtual == "watching")
            llistaAtual = "Watching";
        else if(vlistaAtual.contains("reading"))
            llistaAtual = "Reading";
        else if(vlistaAtual.contains("completed"))
            llistaAtual = "Completed";
        else if(vlistaAtual.contains("onhold"))
            llistaAtual = "On Hold";
        else if(vlistaAtual.contains("dropped"))
            llistaAtual = "Dropped";
        else if(vlistaAtual.contains("plantowatch"))
            llistaAtual = "Plan to Watch";
        else if(vlistaAtual.contains("busca"))
            llistaAtual = "Busca";
        ui->NumPagina->setText(llistaAtual + " - " + QString::number(vlistaSelecionada.size()) +
                               " animes in the list - Page "+QString::number(vpagina)+"/"+
                               QString::number(((vlistaSelecionada.size()-1)/12)+1));
        fcarregaImagensLista();
    }
}

void MainWindow::on_Watching_clicked()
{
    vanimeSelecionado = 0;
    vpagina = 1;
    if(vJanelaManga){
        qDebug() << "Selected list: Reading (Manga)";
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "mangareading");
        vlistaAtual = "mangareading";
        ui->NumPagina->setText("Reading - " + QString::number(vlistaSelecionada.size()) + " mangas in the list - Page "+
                               QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
    }
    else if(vJanelaNovel){
        qDebug() << "Selected list: Reading (Novel)";
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "novelreading");
        vlistaAtual = "novelreading";
        ui->NumPagina->setText("Reading - " + QString::number(vlistaSelecionada.size()) + " novels in the list - Page "+
                               QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
    }
    else if(vlistaAtual.contains("season")){
        vlistaAtual = "season"+QString::number(vanoBuscaAnimes);
        vtipoAtual = "Winter";
        on_botaoBusca_clicked();
    }
    else{
        qDebug() << "Selected list: Watching";
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "watching");
        vlistaAtual = "watching";
        ui->NumPagina->setText("Watching - " + QString::number(vlistaSelecionada.size()) + " animes in the list - Page "+
                               QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
    }
    finfoAnimeSelecionado();
}

void MainWindow::on_Completed_clicked()
{
    vanimeSelecionado = 0;
    vpagina = 1;
    qDebug() << "Selected list: Completed";
    qDebug() << vlistaAtual;
    if(vJanelaManga){
        vlistaAtual = "mangacompleted";
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "mangacompleted");
        ui->NumPagina->setText("Completed - " + QString::number(vlistaSelecionada.size()) + " mangas in the list - Page "+
                               QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
    }
    else if(vJanelaNovel){
        vlistaAtual = "novelcompleted";
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "novelcompleted");
        ui->NumPagina->setText("Completed - " + QString::number(vlistaSelecionada.size()) + " novels in the list - Page "+
                               QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
    }
    else if(vlistaAtual.contains("season")){
        vlistaAtual = "season"+QString::number(vanoBuscaAnimes);
        vtipoAtual = "Spring";
        on_botaoBusca_clicked();
    }
    else{
        vlistaAtual = "completed";
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "completed");
        ui->NumPagina->setText("Completed - " + QString::number(vlistaSelecionada.size()) + " animes in the list - Page "+
                               QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
    }
    finfoAnimeSelecionado();
}

void MainWindow::on_OnHold_clicked()
{
    qDebug() << "Selected list: On Hold";
    vanimeSelecionado = 0;
    vpagina = 1;
    if(vJanelaManga){
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "mangaonhold");
        vlistaAtual = "mangaonhold";
        ui->NumPagina->setText("On Hold - " + QString::number(vlistaSelecionada.size()) + " mangas in the list - Page "+
                               QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
    }
    else if(vJanelaNovel){
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "novelonhold");
        vlistaAtual = "novelonhold";
        ui->NumPagina->setText("On Hold - " + QString::number(vlistaSelecionada.size()) + " novels in the list - Page "+
                               QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
    }
    else if(vlistaAtual.contains("season")){
        vlistaAtual = "season"+QString::number(vanoBuscaAnimes);
        vtipoAtual = "Summer";
        on_botaoBusca_clicked();
    }
    else{
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "onhold");
        vlistaAtual = "onhold";
        ui->NumPagina->setText("On Hold - " + QString::number(vlistaSelecionada.size()) + " animes in the list - Page "+
                               QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
    }
    finfoAnimeSelecionado();
}

void MainWindow::on_Dropped_clicked()
{
    qDebug() << "Selected list: Dropped";
    vanimeSelecionado = 0;
    vpagina = 1;
    if(vJanelaManga){
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "mangadropped");
        vlistaAtual = "mangadropped";
        ui->NumPagina->setText("Dropped - " + QString::number(vlistaSelecionada.size()) + " mangas in the list - Page "+
                               QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
    }
    else if(vJanelaNovel){
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "noveldropped");
        vlistaAtual = "noveldropped";
        ui->NumPagina->setText("Dropped - " + QString::number(vlistaSelecionada.size()) + " novels in the list - Page "+
                               QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
    }
    else if(vlistaAtual.contains("season")){
        vlistaAtual = "season"+QString::number(vanoBuscaAnimes);
        vtipoAtual = "Fall";
        on_botaoBusca_clicked();
    }
    else{
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "dropped");
        vlistaAtual = "dropped";
        ui->NumPagina->setText("Dropped - " + QString::number(vlistaSelecionada.size()) + " animes in the list - Page "+
                               QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
    }
    finfoAnimeSelecionado();
}

void MainWindow::on_PlanToWatch_clicked()
{
    qDebug() << "Selected list: Plan to Watch";
    vanimeSelecionado = 0;
    vpagina = 1;
    if(vJanelaManga){
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "mangaplantoread");
        vlistaAtual = "mangaplantoread";
        ui->NumPagina->setText("Plan to Read - " + QString::number(vlistaSelecionada.size()) + " mangas in the list - Page "+
                               QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
    }
    else if(vJanelaNovel){
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "novelplantoread");
        vlistaAtual = "novelplantoread";
        ui->NumPagina->setText("Plan to Read - " + QString::number(vlistaSelecionada.size()) + " novels in the list - Page "+
                               QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
    }
    else{
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "plantowatch");
        vlistaAtual = "plantowatch";
        ui->NumPagina->setText("Plan to Watch - " + QString::number(vlistaSelecionada.size()) + " animes in the list - Page "+
                               QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
    }
    finfoAnimeSelecionado();
}

void MainWindow::on_botaoProximoEpisodio_clicked()
{
    fupdateMessage("Searching for episode...");
    if(!carquivos->fprocuraEpisodio(vlistaSelecionada[vanimeSelecionado]).isEmpty()){
        if(carquivos->fabreEpisodio(carquivos->fprocuraEpisodio(vlistaSelecionada[vanimeSelecionado]).toUtf8()))
            fupdateMessage("Opening!");
    }
    else
        fupdateMessage("The episode was not found in the computer");
}

void MainWindow::on_botaoAbrePasta_clicked()
{
    qDebug() << "Trying to open anime folder";
    //Pega o caminho da pasta onde o episódio está e converte para o caminho nativo do OS (só pra evitar erros com o nome)
    QByteArray lcaminhoPasta = cconfUsuario->fretornaDiretorioEspecifico(vlistaSelecionada[vanimeSelecionado]->vid.toInt()).toUtf8();
    if(!lcaminhoPasta.isEmpty())
        carquivos->fabreEpisodio(lcaminhoPasta);
    else
        fupdateMessage("The anime folder was not found");
}

void MainWindow::on_botaoBusca_clicked()
{
    if(ui->janelaRotativa->currentIndex() == 0){
        if(!ui->barraBusca->toPlainText().isEmpty()){
            qDebug() << "Searching " << ui->barraBusca->toPlainText();
            vlistaSelecionada = cleitorListaAnimes->fbuscaLista(ui->barraBusca->toPlainText().simplified(), vtipoAtual);
            if(!vlistaSelecionada.isEmpty()){
                vlistaAtual = "busca";
                vanimeSelecionado = 0;
                vpagina = 1;
                ui->NumPagina->setText("Busca  - " + QString::number(vlistaSelecionada.size()) +
                                       " animes in the list - Page "+QString::number(vpagina)+"/"+
                                       QString::number(((vlistaSelecionada.size()-1)/12)+1));
                finfoAnimeSelecionado();
            }
            else{
                qDebug() << ui->barraBusca->toPlainText() << " not found!";
                fupdateMessage(ui->barraBusca->toPlainText()+" not found!");
                vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, vlistaAtual);
            }
        }
        else if(vlistaAtual.contains("season", Qt::CaseInsensitive)){
            QString tempSeason = vlistaAtual;
            vlistaSelecionada = cleitorListaAnimes->sortLista(vordem,vlistaAtual+vtipoAtual);
            if(!vlistaSelecionada.isEmpty()){
                ui->NumPagina->setText(vtipoAtual + " " + tempSeason.remove("season") + "  - " +
                                       QString::number(vlistaSelecionada.size()) + " animes in the list - Page "+
                                       QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
                vlistaAtual = vlistaAtual+vtipoAtual;
                vanimeSelecionado = 0;
                vpagina = 1;
                finfoAnimeSelecionado();
            }
            else{
                tempSeason = tempSeason.remove("season");
                qDebug() << vtipoAtual + " " + tempSeason << " not found!";
                fupdateMessage(vtipoAtual + " " + vlistaAtual.remove("season")+" not found!");
                vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, vlistaAtual);
            }
        }
    }
    else if(ui->janelaRotativa->currentIndex() == 2)
        jtorrent.fprocuraAnimeEspecifico(ui->barraBusca->toPlainText());
}

void MainWindow::on_botaoRefresh_clicked()
{
    if(!vrefreshAcontecendo){
        fupdateMessage("Trying to update list. The program may freeze for a few seconds while reading the new list");
        ui->labelRefreshTimer->setText("Refreshing,,,");
        vrefreshAcontecendo = true;
    }
//    fcarregouListaTeste(false);
    fcarregouListaFalha();
}

void MainWindow::frefreshListas(bool rcheckDownload){
    //Checamos se a conexão com o anilist foi um sucesso. No caso de ter falhado, não rodamos a função de refresh
    if(!rcheckDownload){
        vrefreshAcontecendo = false;
        if(timer->isActive())
            timer->stop();
        timer->setInterval(60000);
        timer->start(); //1 minutos
        timerRefresh->start(1000);
        return;
    }
    if(timer->isActive())
        timer->stop();
    timer->setInterval(600000);
    timer->start(); //10 minutos
    timerRefresh->start(1000);

    fupdateMessage("Anime lists downloaded");

    //Quando rolar a busca, a lista vai resetar. Esse index será usado pra voltar na info do anime que a pessoa estava vendo quando
    //rolou o refresh
    int lanimeAtual = 0;
    int lpaginaAtual = 0;
    bool lbusca = false;
    rcheckDownload = false;
    //Vamos bloquear todos os botões para evitar que o usuário tente ler um anime que saiu da lista
    fupdateMessage("Loading new animes. It will be working again in a few seconds");
    fbloqueiaSinaisBotoes();

    qDebug() << "Refreshing list...";

    //Se não houver nenhum problema no download ou no caso do usuário deletar o arquivo das listas, elas vão estar sempre prontas
    if(vlistaLidaSucesso){
//        timer->start(60000); //1 MINUTO PRA TESTES
        timer->start(600000);
        vtimerSegundos = 59;
        qDebug() << "The anime list have been read successfully";
    }
    else{
        qCritical() << "There was a problem reading the anime list";
        fupdateMessage("Failed to refresh!");
        fliberaSinaisBotoes();
        vrefreshAcontecendo = false;
        return;
    }
    qDebug() << "Checking anime folders";
    fupdateMessage("Searching for animes in the computer");
    cconfUsuario->frecebeListaAnime(cleitorListaAnimes);
    if(!dThread.isRunning()){
//        qDebug() << "dThread não está rodando";
//        dThread.requestInterruption();
//        dThread.wait()
        cconfUsuario->fbuscaPastasThread(dThread);
        cconfUsuario->moveToThread(&dThread);
        dThread.start();
    }
//        dThread.start();

    //Caso o usuário esteja no meio de uma busca quando atualiza a lista, precisamos salvar o index do anime que ele estava vendo.
    lpaginaAtual = vpagina;
    lanimeAtual = vanimeSelecionado;
    if(vlistaAtual.compare("busca", Qt::CaseInsensitive) == 0){
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "watching");
        lbusca = true;
    }
    else
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, vlistaAtual);
    if(lbusca){
        on_botaoBusca_clicked();
        for(int i = 0; i < lpaginaAtual-1; i++){
            on_botaoProximaPagina_clicked();
        }
    }
    if(lanimeAtual < vlistaSelecionada.size() && lanimeAtual > 0){
        vanimeSelecionado = lanimeAtual;
    }
    qDebug() << "The lists were updated";
    fupdateMessage("The lists were updated! Searching animes in the computer.");
    fliberaSinaisBotoes();
    vrefreshAcontecendo = false;

    //Por mais que isso seja útil, faz dar muita merda.
//    ///ISSO GASTA MEMÓRIA PRA PORRA!
//    if(!vbaixaECarregaAnos){
//        if(canilist->fgetListasAnoSeason()){
//            ///ESPECIFICAMENTE AQUI
//            cleitorListaAnimes->fsetListasPorAnoEmThread();
//            vbaixaECarregaAnos = true;
//        }
//    }
}

void MainWindow::fmandaDiretoriosArquivos()
{
    fupdateMessage("All animes in the computer were found");
    carquivos->frecebeDiretorios(cconfUsuario);
}

void MainWindow::favisoErro(const QString &rerro)
{
    fupdateMessage(rerro);
}

void MainWindow::fbloqueiaSinaisBotoes(){
    QList<QPushButton *> listaBotoes = ui->listaAnimes->findChildren<QPushButton *>();
    foreach(QPushButton *psh, listaBotoes){
        psh->blockSignals(true);
    }
}

void MainWindow::fliberaSinaisBotoes(){
    QList<QPushButton *> listaBotoes = ui->listaAnimes->findChildren<QPushButton *>();
    foreach(QPushButton *psh, listaBotoes){
        psh->blockSignals(false);
    }
}

void MainWindow::fatualizaRefreshTimer()
{
//    vlistaBaixada = true;
    if(!vrefreshAcontecendo){
        int lminutos = timer->remainingTime()/60000;
        if(vtimerSegundos == 0)
            ui->labelRefreshTimer->setText(QString::number(lminutos) + ":00");
        else if(vtimerSegundos < 10)
            ui->labelRefreshTimer->setText(QString::number(lminutos) + ":0" + QString::number(vtimerSegundos));
        else
            ui->labelRefreshTimer->setText(QString::number(lminutos) + ":" + QString::number(vtimerSegundos));
        if(vtimerSegundos == 0)
            vtimerSegundos = 59;
        else
            vtimerSegundos--;
    }
}

void MainWindow::on_botaoHome_clicked()
{
    ui->janelaRotativa->setCurrentIndex(0);
}

void MainWindow::on_botaoConfiguracao_clicked()
{
    ui->janelaRotativa->setCurrentIndex(1);
}

void MainWindow::on_botaoNotaMais_clicked()
{
    if(vlistaSelecionada[vanimeSelecionado]->vnotaMediaPessoal.toInt() < 10){
        vlistaSelecionada[vanimeSelecionado]->vnotaMediaPessoal =
                QString::number(vlistaSelecionada[vanimeSelecionado]->vnotaMediaPessoal.toInt()+1);
        ui->labelInfoNota->setText(vlistaSelecionada[vanimeSelecionado]->vnotaMediaPessoal);
        cleitorListaAnimes->fmudaNota(vlistaSelecionada[vanimeSelecionado]->vid,
                                       vlistaSelecionada[vanimeSelecionado]->vnotaMediaPessoal);
        finfoAnimeSelecionado();
        QString lacao = "nota:" + vlistaSelecionada[vanimeSelecionado]->vid;
        QStringList lstringListAcao = lacao.split(':');
        vlistaAcoes.insert(lstringListAcao, QString::number(vlistaSelecionada[vanimeSelecionado]->vnotaMediaPessoal.toInt()));
    }
}

void MainWindow::on_botaoNotaMenos_clicked()
{
    if(vlistaSelecionada[vanimeSelecionado]->vnotaMediaPessoal.toInt() > 0){
        vlistaSelecionada[vanimeSelecionado]->vnotaMediaPessoal =
                QString::number(vlistaSelecionada[vanimeSelecionado]->vnotaMediaPessoal.toInt()-1);
        ui->labelInfoNota->setText(vlistaSelecionada[vanimeSelecionado]->vnotaMediaPessoal);
        cleitorListaAnimes->fmudaNota(vlistaSelecionada[vanimeSelecionado]->vid,
                                       vlistaSelecionada[vanimeSelecionado]->vnotaMediaPessoal);
        finfoAnimeSelecionado();
        QString lacao = "nota:" + vlistaSelecionada[vanimeSelecionado]->vid;
        QStringList lstringListAcao = lacao.split(':');
        vlistaAcoes.insert(lstringListAcao, QString::number(vlistaSelecionada[vanimeSelecionado]->vnotaMediaPessoal.toInt()));
    }
}

void MainWindow::on_botaoProgressoMais_clicked()
{
    int lepisodiosTotais = 0;
    //Caso o número máximo de episódios não seja conhecido, não deve existir um limite;
    if(vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosTotais != "?")
        lepisodiosTotais = vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosTotais.toInt();
    else
        lepisodiosTotais = INT_MAX;
    if(vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosAssistidos.toInt() < lepisodiosTotais){
        vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosAssistidos =
                QString::number(vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosAssistidos.toInt()+1);
        cleitorListaAnimes->fmudaProgresso(vlistaSelecionada[vanimeSelecionado]->vid,
                                       vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosAssistidos);
        finfoAnimeSelecionado();
        QString lacao = "progresso:" + vlistaSelecionada[vanimeSelecionado]->vid;
        QStringList lstringListAcao = lacao.split(':');
        vlistaAcoes.insert(lstringListAcao, QString::number(vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosAssistidos.toInt()));
        if(vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosAssistidos.toInt() ==
                vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosTotais.toInt()){
            ui->boxMudarPraLista->setCurrentIndex(1);
            on_botaoMudarPraLista_clicked();
        }
        else if((vlistaSelecionada[vanimeSelecionado]->vlista.compare("Completed",Qt::CaseInsensitive) != 0)){

            vlistaSelecionada[vanimeSelecionado]->vlista = "Watchig";
            canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "CURRENT");
            cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid,
                                           vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
        }
    }
}

void MainWindow::fAumentaProgressoID(const QString &ridAnime, const QString &repisodioAnime)
{
    fbloqueiaSinaisBotoes();
    QString llistaAtual = vlistaSelecionada[vanimeSelecionado]->vlista;

    int lposicao = cleitorListaAnimes->fbuscaAnimePorIDERetornaPosicao(ridAnime);
    QString llistaAnimeAssistindo = cleitorListaAnimes->fbuscaAnimePorIDERetornaLista(ridAnime);
    //A função vai rodar tudo em uma lista temporária e dar refresh na lista atual, pra mudar o progresso
    QVector<anime*> listaTemp;
    if(llistaAnimeAssistindo.isEmpty())
        return;
    else{
        //Olha todas as listas, exceto completed. Não tem sentido aumentar o progresso de algo já completo.
        if(llistaAnimeAssistindo.compare("Watching",Qt::CaseInsensitive) == 0){
            listaTemp = cleitorListaAnimes->retornaListaWatching();
        }
        else if(llistaAnimeAssistindo.compare("On Hold",Qt::CaseInsensitive) == 0){
            listaTemp = cleitorListaAnimes->retornaListaOnHold();
        }
        else if(llistaAnimeAssistindo.compare("Dropped",Qt::CaseInsensitive) == 0){
            listaTemp = cleitorListaAnimes->retornaListaDropped();
        }
        else if(llistaAnimeAssistindo.compare("Plan to Watch",Qt::CaseInsensitive) == 0){
            listaTemp = cleitorListaAnimes->retornaListaPlanToWatch();
        }
        else{
            bool ok;
            //Checa se a lista é um número válido
            llistaAnimeAssistindo.toInt(&ok);
            if(ok){
                listaTemp = cleitorListaAnimes->fleListaAno(llistaAnimeAssistindo.toInt());
            }
        }
    }
    if(listaTemp.isEmpty())
        return;
    else if(lposicao != -1 && listaTemp[lposicao]->vnumEpisodiosAssistidos.toInt() < repisodioAnime.toInt()){
        for(int i = 0; i < repisodioAnime.toInt()-listaTemp[lposicao]->vnumEpisodiosAssistidos.toInt(); i++){
            int lepisodiosTotais = 0;
            //Caso o número máximo de episódios não seja conhecido, não deve existir um limite;
            if(listaTemp[lposicao]->vnumEpisodiosTotais != "?")
                lepisodiosTotais = listaTemp[lposicao]->vnumEpisodiosTotais.toInt();
            else
                lepisodiosTotais = INT_MAX;
            if(listaTemp[lposicao]->vnumEpisodiosAssistidos.toInt() < lepisodiosTotais){
                listaTemp[lposicao]->vnumEpisodiosAssistidos =
                        QString::number(listaTemp[lposicao]->vnumEpisodiosAssistidos.toInt()+1);
                cleitorListaAnimes->fmudaProgresso(listaTemp[i]->vid,
                                               listaTemp[i]->vnumEpisodiosAssistidos);
                finfoAnimeSelecionado();
                QString lacao = "progresso:" + listaTemp[lposicao]->vid;
                QStringList lstringListAcao = lacao.split(':');
                vlistaAcoes.insert(lstringListAcao,
                                   QString::number(listaTemp[lposicao]->vnumEpisodiosAssistidos.toInt()));
                if(listaTemp[lposicao]->vnumEpisodiosAssistidos.toInt() ==
                        listaTemp[lposicao]->vnumEpisodiosTotais.toInt()){
                    listaTemp[lposicao]->vlista = "Completed";
                    canilist->fmudaLista(listaTemp[lposicao]->vid.toInt(), "COMPLETED");
                    cleitorListaAnimes->fmudaLista(listaTemp[lposicao]->vid,
                                                   listaTemp[lposicao]->vlista, "anime");
                }
                //Caso esteja vendo um anime de, por exemplo, plan to watch, manda pra lista watching
                //Se estiver vendo um anime que não está na lista, ele é adicionado em Watching também.
                else if((llistaAnimeAssistindo.compare("Completed",Qt::CaseInsensitive) != 0)){

                    listaTemp[lposicao]->vlista = "Watchig";
                    canilist->fmudaLista(listaTemp[lposicao]->vid.toInt(), "CURRENT");
                    cleitorListaAnimes->fmudaLista(listaTemp[lposicao]->vid,
                                                   listaTemp[lposicao]->vlista, vtipoAtual);
                }
            }
        }
    }
    //Aqui rola o refresh depois de ter mudado a lista temporária
    if(llistaAnimeAssistindo.compare("On Hold",Qt::CaseInsensitive) == 0){
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "onhold");
    }
    else if(llistaAnimeAssistindo.compare("Dropped",Qt::CaseInsensitive) == 0){
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "dropped");
    }
    else if(llistaAnimeAssistindo.compare("Plan to Watch",Qt::CaseInsensitive) == 0){
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "plantowatch");
    }
    else if(llistaAnimeAssistindo.compare("Completed",Qt::CaseInsensitive) == 0){
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "completed");
    }
    else{
        vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, "watching");
    }
    if(llistaAtual.compare(llistaAnimeAssistindo, Qt::CaseInsensitive) == 0){
        if(vlistaSelecionada.size() > vanimeSelecionado && vanimeSelecionado != 0){
            vanimeSelecionado--;
            if((vanimeSelecionado < 12*(vpagina-1)) && vpagina != 1){
                vpagina--;
            }
        }
        else{
            vanimeSelecionado = 0;
            vpagina = 1;
        }
    }
    fliberaSinaisBotoes();
}

void MainWindow::fgetConfigurations()
{
    fgetStreamLanguages();
    fgetMediaPlayersFromList();
    if(jconfig.fretornaBaixaQualidade().contains("yes"))
        vusarImagensBaixaQualidade = true;
    if(jconfig.fretornaDownloadAutomatico().contains("yes")){
        timerTorrent->start(jconfig.fretornaTempoDownload().toInt()*10000);
        connect(timerTorrent, &QTimer::timeout, &jtorrent, QOverload<>::of(&janelatorrent::fautoDownload));
    }
    fatualizaFontes();
}

void MainWindow::fsetDownloadImagensAnimesPorAno()
{
    vprogressoLoadingBar = 0;
    vtotalAnimesLoadingBar = vlistaSelecionada.size();
    fupdateMessage("Downloading and updating images. It can take a few minutes to download every image");
    if(vdownloadImagensAcabou && !vbaixandoImagensAno){
        if(canilist->fgetListasAnoSeason()){
            if(!vdownloadImagensAnos.isEmpty())
                cfiledownloader->fdownloadPorAno(vdownloadImagensAnos.takeFirst());
            else
                cfiledownloader->fdownloadPorAno(vanoBuscaAnimes);
            vbaixandoImagensAno = true;
            if(vdownloadImagensAnos.isEmpty()){
                if(timerChecaDownloadPorAno->isActive())
                    timerChecaDownloadPorAno->stop();
            }
        }
    }
    else{
        if(!timerChecaDownloadPorAno->isActive())
            timerChecaDownloadPorAno->start(5000);
        if(!vdownloadImagensAnos.contains(vanoBuscaAnimes))
            vdownloadImagensAnos.append(vanoBuscaAnimes);
    }
}

void MainWindow::fsalvaNomesAlternativos()
{
    QFile larquivo("Configurações/nomesAlternativosAnimes.txt");
    if(larquivo.open(QIODevice::WriteOnly)){
        QTextStream lstreamTexto(&larquivo);
        lstreamTexto.setCodec("UTF-8");
        foreach(QString key, vcustomNomesAlternativos.keys()){
            lstreamTexto << key << ";" << vcustomNomesAlternativos[key].join(";").trimmed() << endl;
        }
        larquivo.close();
    }
}

void MainWindow::fleNomesAlternativos()
{
    QFile larquivo("Configurações/nomesAlternativosAnimes.txt");
    if(larquivo.size() == 0)
        return;
    if(larquivo.open(QIODevice::ReadOnly)){
        while(!larquivo.atEnd()){
            QString lstreamTexto = larquivo.readLine();
            QStringList lnomesAlternativos = lstreamTexto.split(";");
            lnomesAlternativos.last() = lnomesAlternativos.last().trimmed();
            QString id = lnomesAlternativos.takeFirst();
            vcustomNomesAlternativos.insert(id,lnomesAlternativos);
            cleitorListaAnimes->finsereNomeAlternativo(id,lnomesAlternativos);
        }
        larquivo.close();
    }
}

void MainWindow::fresetaRequestCounter()
{
    vrateLimitRequestsAnilist = 0;
    timerMaxAnilistRequest.singleShot(61000, this, &MainWindow::fresetaRequestCounter);
}

void MainWindow::fatualizaFontes()
{
    QStringList fontes = jconfig.fretornaFontes().split(";");
    fontName = "\""+fontes.takeFirst()+"\"";
    fontSizeTitulo = fontes.takeFirst();
    fontSizeTituloAlternativo = fontes.takeFirst();
    fontSizeStatus = fontes.takeFirst();
    fontSizeInfo = fontes.takeFirst();
    fontSizeMensagem = fontes.takeFirst();
    vfonteTitulo = "background: transparent;font: 75 "+fontSizeTitulo+"pt " + fontName +
            ";font-weight: bold; color: rgb(20, 20, 20);";
    vfonteSubtitulo = "background: transparent;font: 75 "+fontSizeTituloAlternativo+"pt " + fontName +
            "; color: rgb(20, 20, 20);";
    vfonteStatus = "background: transparent;font: 75 "+fontSizeStatus+"pt " + fontName +
            "; color: rgb(20, 20, 20);";
    vfonteInfo = "background: transparent;font: 75 "+fontSizeInfo+"pt " + fontName +
            ";font-weight: bold; color: rgb(20, 20, 20);";
    vfonteMensagem = "background: transparent;font: 75 "+fontSizeMensagem+"pt " + fontName +
            ";color: rgb(20, 20, 20);";


    ui->labelInfoNomeAnime->setStyleSheet(vfonteTitulo);
    ui->labelInfoNomeAnimeIngles->setStyleSheet(vfonteSubtitulo);
    ui->labelInfoStatusTop->setStyleSheet(vfonteStatus);
    ui->labelInfoNotaTop->setStyleSheet(vfonteStatus);
    ui->labelInfoMediaTop->setStyleSheet(vfonteStatus);
    ui->labelInfoSeasonTop->setStyleSheet(vfonteStatus);
    ui->labelInfoAirTop->setStyleSheet(vfonteStatus);
    ui->labelInfoReleasedTop->setStyleSheet(vfonteStatus);
    ui->labelInfoTypeTop->setStyleSheet(vfonteStatus);
    ui->labelInfoStatus->setStyleSheet(vfonteInfo);
    ui->labelInfoNota->setStyleSheet(vfonteInfo);
    ui->labelInfoMediaSite->setStyleSheet(vfonteInfo);
    ui->labelInfoSeason->setStyleSheet(vfonteInfo);
    ui->labelInfoRelease->setStyleSheet(vfonteInfo);
    ui->labelInfoEpisodiosLancados->setStyleSheet(vfonteInfo);
    ui->labelInfoTipo->setStyleSheet(vfonteInfo);
    ui->labelInfoSinopseTop->setStyleSheet(vfonteStatus);
    ui->labelLoading->setStyleSheet(vfonteMensagem);
    ui->labelRefreshTimer->setStyleSheet(vfonteMensagem);
    ui->labelMensagem->setStyleSheet(vfonteMensagem);
    ui->labelMensagemAnimeAssistindo->setStyleSheet(vfonteMensagemAnime);
    //Falta o nick e a mensagem do anime, apenas.
}


void MainWindow::fgetMediaPlayersFromList()
{
    vPlayers = jconfig.fretornaPlayers();
}

void MainWindow::fgetStreamLanguages()
{
    vStreamLanguages = jconfig.fretornaStreamLanguages();
}

void MainWindow::on_botaoProgressoMenos_clicked()
{
    if(vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosAssistidos.toInt() > 0){
        vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosAssistidos =
                QString::number(vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosAssistidos.toInt()-1);
        cleitorListaAnimes->fmudaProgresso(vlistaSelecionada[vanimeSelecionado]->vid,
                                       vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosAssistidos);
        finfoAnimeSelecionado();
        QString lacao = "progresso:" + vlistaSelecionada[vanimeSelecionado]->vid;
        QStringList lstringListAcao = lacao.split(':');
        vlistaAcoes.insert(lstringListAcao, QString::number(vlistaSelecionada[vanimeSelecionado]->vnumEpisodiosAssistidos.toInt()));
    }
}

void MainWindow::fatualizaAnilist(){
    if(vlistaAcoes.isEmpty())
        return;
    QString nome;
    foreach (QStringList key, vlistaAcoes.keys()) {
        nome = cleitorListaAnimes->fbuscaAnimePorIDERetornaTitulo(key.at(1));
        if(vrateLimitRequestsAnilist == 90)
            return;
        else if(key.at(0) == "nota"){
            if(canilist->fmudaNota(key.at(1).toInt(), vlistaAcoes[key].toInt())){
                qDebug() << QString("Change: Score - " + nome + " - " + vlistaAcoes[key]
                                     + " - " + QDateTime::currentDateTime().toString());
                vhistoricoAnilist.append(QString("Change: Score - " + nome + " - " + vlistaAcoes[key]
                                                 + " - " + QDateTime::currentDateTime().toString()));
                vlistaAcoes.remove(key);
            }
            else
                qDebug() << "Couldn't reach anilist server. Trying again in 10 seconds.";
        }
        else if(key.at(0) == "progresso"){
            if(canilist->fmudaProgresso(key.at(1).toInt(), vlistaAcoes[key].toInt())){
                qDebug() << QString("Change: Progress - " + vlistaSelecionada[vanimeSelecionado]->vnome +
                                    " - " + vlistaAcoes[key] + " - " + QDateTime::currentDateTime().toString());
                vhistoricoAnilist.append(QString("Change: Progress - " + vlistaSelecionada[vanimeSelecionado]->vnome +
                                                 " - " + vlistaAcoes[key] + " - " + QDateTime::currentDateTime().toString()));
                vlistaAcoes.remove(key);
            }
            else
                qDebug() << "Couldn't reach anilist server. Trying again in 10 seconds.";
        }
        else if(key.at(0) == "lista"){
            if(canilist->fmudaLista(key.at(1).toInt(), vlistaAcoes[key])){
                qDebug() << QString("Change: List - " + vlistaSelecionada[vanimeSelecionado]->vnome +
                                    " - " + vlistaAcoes[key] + " - " + QDateTime::currentDateTime().toString());
                vhistoricoAnilist.append(QString("Change: List - " + vlistaSelecionada[vanimeSelecionado]->vnome +
                                                 " - " + vlistaAcoes[key] + " - " + QDateTime::currentDateTime().toString()));
                vlistaAcoes.remove(key);
            }
            else
                qDebug() << "Couldn't reach anilist server. Trying again in 10 seconds.";
        }
        else if(key.at(0) == "remove"){
            if(canilist->fexcluiAnime(key.at(1).toInt())){
                qDebug() << QString("Change: Remove - " + vlistaSelecionada[vanimeSelecionado]->vnome +
                                    " - " + vlistaAcoes[key] + " - " + QDateTime::currentDateTime().toString());
                vhistoricoAnilist.append(QString("Change: Remove - " + vlistaSelecionada[vanimeSelecionado]->vnome +
                                                 " - " + vlistaAcoes[key] + " - " + QDateTime::currentDateTime().toString()));
                vlistaAcoes.remove(key);
            }
            else
                qDebug() << "Couldn't reach anilist server. Trying again in 10 seconds.";
        }
        vrateLimitRequestsAnilist++;
    }
    jconfig.fsetHistorico(vhistoricoAnilist);
}

void MainWindow::on_botaoMudarPraLista_clicked()
{
    qDebug() << "Updating anime list";
    if(ui->boxMudarPraLista->currentText().compare(vlistaAtual, Qt::CaseInsensitive) == 0)
        fupdateMessage("Já está na lista correta!");
    else if(ui->boxMudarPraLista->currentText().compare("Watching") == 0){
        vlistaSelecionada[vanimeSelecionado]->vlista = "Watching";
        //Atualiza o anilist e a lista local de animes
        canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "CURRENT");
        cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid, vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
    }
    else if(ui->boxMudarPraLista->currentText().compare("Reading") == 0 && vtipoAtual.compare("manga") == 0){
        vlistaSelecionada[vanimeSelecionado]->vlista = "Reading(Manga)";
        canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "CURRENT");
        cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid, vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
    }
    else if(ui->boxMudarPraLista->currentText().compare("Reading") == 0 && vtipoAtual.compare("novel") == 0){
        vlistaSelecionada[vanimeSelecionado]->vlista = "Reading(Novel)";
        canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "CURRENT");
        cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid, vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
    }
    else if(ui->boxMudarPraLista->currentText().compare("Completed") == 0 && vtipoAtual.compare("anime") == 0){
        vlistaSelecionada[vanimeSelecionado]->vlista = "Completed";
        canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "COMPLETED");
        cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid, vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
    }
    else if(ui->boxMudarPraLista->currentText().compare("Completed") == 0 && vtipoAtual.compare("manga") == 0){
        vlistaSelecionada[vanimeSelecionado]->vlista = "Completed(Manga)";
        canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "COMPLETED");
        cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid, vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
    }
    else if(ui->boxMudarPraLista->currentText().compare("Completed") == 0 && vtipoAtual.compare("novel") == 0){
        vlistaSelecionada[vanimeSelecionado]->vlista = "Completed(Novel)";
        canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "COMPLETED");
        cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid, vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
    }
    else if(ui->boxMudarPraLista->currentText().compare("On Hold") == 0 && vtipoAtual.compare("anime") == 0){
        vlistaSelecionada[vanimeSelecionado]->vlista = "On Hold";
        canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "PAUSED");
        cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid, vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
    }
    else if(ui->boxMudarPraLista->currentText().compare("On Hold") == 0 && vtipoAtual.compare("manga") == 0){
        vlistaSelecionada[vanimeSelecionado]->vlista = "On Hold(Manga)";
        canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "PAUSED");
        cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid, vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
    }
    else if(ui->boxMudarPraLista->currentText().compare("On Hold") == 0 && vtipoAtual.compare("novel") == 0){
        vlistaSelecionada[vanimeSelecionado]->vlista = "On Hold(Novel)";
        canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "PAUSED");
        cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid, vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
    }
    else if(ui->boxMudarPraLista->currentText().compare("Dropped") == 0 && vtipoAtual.compare("anime") == 0){
        vlistaSelecionada[vanimeSelecionado]->vlista = "Dropped";
        canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "DROPPED");
        cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid, vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
    }
    else if(ui->boxMudarPraLista->currentText().compare("Dropped") == 0 && vtipoAtual.compare("manga") == 0){
        vlistaSelecionada[vanimeSelecionado]->vlista = "Dropped(Manga)";
        canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "DROPPED");
        cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid, vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
    }
    else if(ui->boxMudarPraLista->currentText().compare("Dropped") == 0 && vtipoAtual.compare("novel") == 0){
        vlistaSelecionada[vanimeSelecionado]->vlista = "Dropped(Novel)";
        canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "DROPPED");
        cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid, vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
    }
    else if(ui->boxMudarPraLista->currentText().compare("Plan to Watch") == 0){
        vlistaSelecionada[vanimeSelecionado]->vlista = "Plan to Watch";
        canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "PLANNING");
        cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid, vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
    }
    else if(ui->boxMudarPraLista->currentText().compare("Plan to Read") == 0 && vtipoAtual.compare("manga") == 0){
        vlistaSelecionada[vanimeSelecionado]->vlista = "Plan to Read(Manga)";
        canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "PLANNING");
        cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid, vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
    }
    else if(ui->boxMudarPraLista->currentText().compare("Plan to Read") == 0 && vtipoAtual.compare("novel") == 0){
        vlistaSelecionada[vanimeSelecionado]->vlista = "Plan to Read(Novel)";
        canilist->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid.toInt(), "PLANNING");
        cleitorListaAnimes->fmudaLista(vlistaSelecionada[vanimeSelecionado]->vid, vlistaSelecionada[vanimeSelecionado]->vlista, vtipoAtual);
    }
    vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, vlistaAtual);
    if(vlistaSelecionada.size() > vanimeSelecionado && vanimeSelecionado != 0){
        vanimeSelecionado--;
        if(vanimeSelecionado < 12*(vpagina-1) && vpagina != 1){
            vpagina--;
        }
    }
    else{
        vanimeSelecionado = 0;
        vpagina = 1;
    }
    finfoAnimeSelecionado();
//    fatualizaAnilist();
}

void MainWindow::on_botaoAnilist_clicked()
{
    QDesktopServices::openUrl(QUrl(vlistaSelecionada[vanimeSelecionado]->vsiteAnilist));
}

void MainWindow::on_botaoCrunchyroll_clicked()
{
    if(!QDesktopServices::openUrl(QUrl(vlistaSelecionada[vanimeSelecionado]->vstreamCrunchyroll)))
        fupdateMessage("A página do Crunchyroll não foi encontrada");
}

void MainWindow::on_botaoTorrent_clicked()
{
    ui->janelaRotativa->setCurrentIndex(2);
}

void MainWindow::on_botaoRemoverdaLista_clicked()
{
    qDebug() << "Removing " << vlistaSelecionada[vanimeSelecionado]->vnome << "from list";
    vordem = "";
    QString lacao = "remove:" + vlistaSelecionada[vanimeSelecionado]->vid;
    QStringList lstringListAcao = lacao.split(':');
    vlistaAcoes.insert(lstringListAcao, "null");
    cleitorListaAnimes->fdeletedaLista(vlistaSelecionada[vanimeSelecionado]->vid);
    vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, vlistaAtual);
    if(vlistaSelecionada.size() > vanimeSelecionado && vanimeSelecionado != 0){
        vanimeSelecionado--;
        if(vanimeSelecionado < 12*(vpagina-1) && vpagina != 1){
            vpagina--;
        }
    }
    else
        vanimeSelecionado = -1;
    finfoAnimeSelecionado();
//    fatualizaAnilist();
}

void MainWindow::fretryAnilist()
{
    ///Aqui era dthread, mudei 2/4 - 13h
    if(cThread.isRunning()){
        cThread.requestInterruption();
//        cThread.wait();
        //Acho que é esse wait que tá fazendo crashar
    }
    canilist->frecebeAutorizacao(jconfig.fretornaUsuario(),jconfig.fretornaCodigoAutorizacao());
//    canilist->fbaixaListaThread(cThread);
//    canilist->moveToThread(&cThread);
    fcarregouListaTeste(false);
//    fcarregouListaFalha();
}

void MainWindow::on_botaoOrdemAlfabetica_clicked()
{
    qDebug() << "Trying to sort list by alphabetical order";
    if(vordem.compare("cnome", Qt::CaseInsensitive) == 0)
        vordem = "dnome";
    else
        vordem = "cnome";
    vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, vlistaAtual);
    if(!vlistaSelecionada.isEmpty()){
        vanimeSelecionado = 0;
        vpagina = 1;
        ui->NumPagina->setText(vlistaAtual + " - " + QString::number(vlistaSelecionada.size()) + " animes in the list - Page "+QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoOrdemProgresso_clicked()
{
    qDebug() << "Trying to sort list by progress order";
    if(vordem.compare("cprogresso", Qt::CaseInsensitive) == 0)
        vordem = "dprogresso";
    else
        vordem = "cprogresso";
    vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, vlistaAtual);
    if(!vlistaSelecionada.isEmpty()){
        vanimeSelecionado = 0;
        vpagina = 1;
        ui->NumPagina->setText(vlistaAtual + " - " + QString::number(vlistaSelecionada.size()) + " animes in the list - Page "+QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoOrdemScore_clicked()
{
    qDebug() << "Trying to sort list by score order";
    if(vordem.compare("cnota", Qt::CaseInsensitive) == 0)
        vordem = "dnota";
    else
        vordem = "cnota";
    vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, vlistaAtual);
    if(!vlistaSelecionada.isEmpty()){
        vanimeSelecionado = 0;
        vpagina = 1;
        ui->NumPagina->setText(vlistaAtual + " - " + QString::number(vlistaSelecionada.size()) + " animes in the list - Page "+QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoOrdemRelease_clicked()
{
    qDebug() << "Trying to sort list by release order";
    if(vordem.compare("cdata", Qt::CaseInsensitive) == 0)
        vordem = "ddata";
    else
        vordem = "cdata";
    vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, vlistaAtual);
    if(!vlistaSelecionada.isEmpty()){
        vanimeSelecionado = 0;
        vpagina = 1;
        ui->NumPagina->setText(vlistaAtual + " - " + QString::number(vlistaSelecionada.size()) + " animes in the list - Page "+QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
        finfoAnimeSelecionado();
    }
}

void MainWindow::on_botaoOrdemType_clicked()
{
    qDebug() << "Trying to sort list by type order";
    if(vordem.compare("cformato", Qt::CaseInsensitive) == 0)
        vordem = "dformato";
    else
        vordem = "cformato";
    vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, vlistaAtual);
    if(!vlistaSelecionada.isEmpty()){
        vanimeSelecionado = 0;
        vpagina = 1;
        ui->NumPagina->setText(vlistaAtual + " - " + QString::number(vlistaSelecionada.size()) + " animes in the list - Page "+QString::number(vpagina)+"/"+QString::number(((vlistaSelecionada.size()-1)/12)+1));
        finfoAnimeSelecionado();
    }
}


//Estou recriando isso em QML, mas vai levar um tempo
void MainWindow::fmudaResolucao(){
//    this->logicalDpiX() =
    QPointer<QScreen> screen = QGuiApplication::primaryScreen();
//    screen->logicalDotsPerInch();
    QRect  screenGeometry = screen->geometry();
    int height = screenGeometry.height();
    int width = screenGeometry.width();
//    width = 1360;
//    height = 768;
    QObjectList objectList = ui->centralwidget->children();
    //1000 é a altura da janela e 1080 a altura do monitor, em pixels. Preciso fazer isso para não deixar a janela ficar atrás do menu
    //iniciar. O windows aceita a janela ficar atrás, mas imagino que terão OS que não irão aceitar.
    int objheight = qCeil(static_cast<int>(height*(static_cast<double>(1000)/1080)));
    //Comparamos o tamanho da página original com a nova resolução, para definir em quantos % deveremos aumentar ou diminuir os
    //widgets
    compHeight = static_cast<double>(objheight)/1000;
    compWidth = static_cast<double>(width)/1920;
    foreach(QObject *wid, objectList){
        //Pego a posição relativa dos objetos
        double a = static_cast<double>(thisWid->pos().rx())/1920;
        double b = static_cast<double>(thisWid->pos().ry())/1000;
        //Variáveis que não precisavam existir. Posso não usar elas, mas deixaria o código mais difícil de se ler
        double objWid = static_cast<double>(this->findChild<QWidget *>(wid->objectName())->width());
        double objHei = static_cast<double>(this->findChild<QWidget *>(wid->objectName())->height());
        //Multiplico o tamanho real do objeto pelo tamanho relativo do objeto.
        //Ex: Se o objeto ocupa, originalmente, 5% do tamanho da página, faço o novo objeto ocupar 5% do tamanho da página com a
        //Nova resolução
        thisWid->resize(qCeil(static_cast<int>(compWidth*objWid)),
                                                              qCeil(static_cast<int>(compHeight*objHei)));
        //Faço os objetos entrarem na posição certa;
        thisWid->move(qCeil(static_cast<int>(a*width)), qCeil(static_cast<int>(b*objheight)));
    }
    //Essa função funciona muito bem, mas algumas coisas ficam com 1 ou 2 pixels fora do lugar
    int widheight = qCeil(static_cast<int>(height*(static_cast<double>(821)/1080)));
    //Dou resize na janela
    double comprimentoJanela = static_cast<double>(width)/1920;
    double alturaJanela = static_cast<double>(widheight)/821;
    //Pego a posição e o tamanho do label que vou usar como referência, já com a nova resolução
    double posxLabelAnime = static_cast<double>(ui->imagemAnime00->pos().rx())/1920;
    double posyLabelAnime = static_cast<double>(ui->imagemAnime00->pos().ry())/821;
    double larguraLabelAnime = static_cast<double>(ui->imagemAnime00->width());
    double alturaLabelAnime = static_cast<double>(ui->imagemAnime00->height());
    //Dou resize nos labeis base pra nova resolução
    ui->imagemAnime00->resize(static_cast<int>(comprimentoJanela*larguraLabelAnime),
                             static_cast<int>(alturaJanela*alturaLabelAnime));
    ui->imagemAnime00->move(static_cast<int>(posxLabelAnime*width),
                           static_cast<int>(posyLabelAnime*widheight));
    ui->labelFundoAnime00Titulo->resize(static_cast<int>(comprimentoJanela*ui->labelFundoAnime00Titulo->width()),
                             static_cast<int>(alturaJanela*ui->labelFundoAnime00Titulo->height()));
    ui->labelFundoAnime00Titulo->move(static_cast<int>((static_cast<double>(ui->labelFundoAnime00Titulo->pos().rx())/1920)*width),
                           static_cast<int>((static_cast<double>(ui->labelFundoAnime00Titulo->pos().ry())/821)*widheight));
    ui->labelAnime00Titulo->resize(static_cast<int>(comprimentoJanela*ui->labelAnime00Titulo->width()),
                             static_cast<int>(alturaJanela*ui->labelAnime00Titulo->height()));
    ui->labelAnime00Titulo->move(static_cast<int>((static_cast<double>(ui->labelAnime00Titulo->pos().rx())/1920)*width),
                           static_cast<int>((static_cast<double>(ui->labelAnime00Titulo->pos().ry())/821)*widheight));
    ui->imagemAnimeGrande->resize(static_cast<int>(comprimentoJanela*ui->imagemAnimeGrande->width()),
                             static_cast<int>(alturaJanela*ui->imagemAnimeGrande->height()));
    ui->imagemAnimeGrande->move(static_cast<int>((static_cast<double>(ui->imagemAnimeGrande->pos().rx())/1920)*width),
                           static_cast<int>((static_cast<double>(ui->imagemAnimeGrande->pos().ry())/821)*widheight));

    //Começo a aplicar isso aos outros labels. Pra isso, preciso fazer uma lista com todos os labels disponíveis
    QWidgetList widgetList = ui->listaAnimes->findChildren<QWidget *>();
    //Criar um loop para iterá-los
    foreach(QObject *wid, widgetList){
        //E ver qual label é qual, pra saber como mudá-los
        //No caso, se eles forem labeis da mesma categoria que o que escolhi pra referência, eles devem ter sempre o mesmo tamanho
        //Mas diferentes posições
        if((thisWid->objectName().contains("botaoAnime", Qt::CaseInsensitive)
                || (thisWid->objectName().contains("imagemAnime", Qt::CaseInsensitive)
                && !thisWid->objectName().contains("grande", Qt::CaseInsensitive)))
                && !thisWid->objectName().contains("assistindo", Qt::CaseInsensitive)){
            this->findChild<QWidget *>(wid->objectName())->resize(static_cast<int>(comprimentoJanela*larguraLabelAnime),
                                                                  static_cast<int>(alturaJanela*alturaLabelAnime));
            QString vposicaoGrade;
            if(thisWid->objectName().contains("botaoAnime", Qt::CaseInsensitive))
                vposicaoGrade = thisWid->objectName().remove("botaoAnime");
            else if(thisWid->objectName().contains("imagemAnime", Qt::CaseInsensitive))
                vposicaoGrade = thisWid->objectName().remove("imagemAnime");
            //Se for um múltiplo de 3, fica na primeira coluna
            if(vposicaoGrade.toInt()%3 == 0)
                thisWid->move(ui->imagemAnime00->x(),
                    static_cast<int>(ui->imagemAnime00->y()+((ui->imagemAnime00->height()+2)*vposicaoGrade.toInt()/3)));
            else if((vposicaoGrade.toInt()-1)%3 == 0)
                thisWid->move(ui->labelFundoAnime00Titulo->x()
                                                                    +ui->labelFundoAnime00Titulo->width()+10,
                    static_cast<int>(ui->imagemAnime00->y()+((ui->imagemAnime00->height()+2)*(vposicaoGrade.toInt()-1)/3)));
            else if((vposicaoGrade.toInt()-2)%3 == 0)
                thisWid->move((ui->labelFundoAnime00Titulo->x() + ui->imagemAnime00->width()
                                                                    +(ui->labelFundoAnime00Titulo->width()+10)*2),
                    static_cast<int>(ui->imagemAnime00->y()+((ui->imagemAnime00->height()+2)*(vposicaoGrade.toInt()-2)/3)));
        }
        //Labelfundo e label anime são separados por motivos de tamanho do label
        else if(thisWid->objectName().contains("labelFundoAnime", Qt::CaseInsensitive)){
            if(thisWid->objectName().contains("titulo", Qt::CaseInsensitive))
                thisWid->resize(ui->labelFundoAnime00Titulo->width(),
                                                                      ui->labelFundoAnime00Titulo->height());
            else
                thisWid->resize(ui->labelFundoAnime00Titulo->width(),
                                                                      ui->labelFundoAnime00Titulo->height()/2);

            QString vposicaoGrade = this->findChild<QWidget *>(wid->objectName())->objectName().remove("labelFundoAnime");
            int ypos = 0;
            if(thisWid->objectName().contains("titulo", Qt::CaseInsensitive)){
                vposicaoGrade = vposicaoGrade.remove("titulo", Qt::CaseInsensitive);
                ypos = ui->labelFundoAnime00Titulo->y();
            }
            else if(thisWid->objectName().contains("progresso", Qt::CaseInsensitive)){
                vposicaoGrade = vposicaoGrade.remove("progresso", Qt::CaseInsensitive);
                ypos = ui->labelFundoAnime00Titulo->y()+ui->labelFundoAnime00Titulo->height()+1;
            }
            else if(thisWid->objectName().contains("nota", Qt::CaseInsensitive)){
                vposicaoGrade = vposicaoGrade.remove("nota", Qt::CaseInsensitive);
                ypos = static_cast<int>(ui->labelFundoAnime00Titulo->y()+(ui->labelFundoAnime00Titulo->height()*1.5)+2);
            }
            else if(thisWid->objectName().contains("lista", Qt::CaseInsensitive)){
                vposicaoGrade = vposicaoGrade.remove("lista", Qt::CaseInsensitive);
                ypos = ui->labelFundoAnime00Titulo->y()+ui->labelFundoAnime00Titulo->height()+
                        ((ui->labelFundoAnime00Progresso->height())*2)+1;
            }
            else{
                ypos = ui->labelFundoAnime00Titulo->y();
            }
            if(vposicaoGrade.toInt()%3 == 0){
                thisWid->move(ui->labelFundoAnime00Titulo->x(),
                    static_cast<int>(ypos+((2+ui->imagemAnime00->height())*
                        (vposicaoGrade.toInt()/3))));
            }
            else if((vposicaoGrade.toInt()-1)%3 == 0)
                thisWid->move(ui->labelFundoAnime00Titulo->x()
                    +ui->labelFundoAnime00Titulo->width()+10
                    +ui->imagemAnime00->width(),
                    static_cast<int>(ypos+((2+ui->imagemAnime00->height())*(vposicaoGrade.toInt()/3))));
            else if((vposicaoGrade.toInt()-2)%3 == 0)
               thisWid->move(ui->labelFundoAnime00Titulo->x()
                    +((ui->labelFundoAnime00Titulo->width()+10
                    +ui->imagemAnime00->width())*2),
                    static_cast<int>(ypos+((2+ui->imagemAnime00->height())*(vposicaoGrade.toInt()/3))));
        }
        //Agora começa a mudar os textos
        else if(thisWid->objectName().contains("labelAnime", Qt::CaseInsensitive) &&
                !thisWid->objectName().contains("assistindo", Qt::CaseInsensitive)){
            if(thisWid->objectName().contains("titulo", Qt::CaseInsensitive))
                thisWid->resize(ui->labelAnime00Titulo->width(),ui->labelAnime00Titulo->height());
            else
                thisWid->resize(ui->labelAnime00Titulo->width(),ui->labelAnime00Titulo->height()/2);

            QString vposicaoGrade = thisWid->objectName().remove("labelAnime");
            int ypos = 0;
            if(thisWid->objectName().contains("titulo", Qt::CaseInsensitive)){
                vposicaoGrade = vposicaoGrade.remove("titulo", Qt::CaseInsensitive);
                ypos = ui->labelAnime00Titulo->y();
            }
            else if(thisWid->objectName().contains("progresso", Qt::CaseInsensitive)){
                vposicaoGrade = vposicaoGrade.remove("progresso", Qt::CaseInsensitive);
                ypos = ui->labelAnime00Titulo->y()+ui->labelAnime00Titulo->height()+2;
            }
            else if(thisWid->objectName().contains("nota", Qt::CaseInsensitive)){
                vposicaoGrade = vposicaoGrade.remove("nota", Qt::CaseInsensitive);
                ypos = static_cast<int>(ui->labelAnime00Titulo->y()+(ui->labelAnime00Titulo->height()*1.5)+4);
            }
            else if(thisWid->objectName().contains("lista", Qt::CaseInsensitive)){
                vposicaoGrade = vposicaoGrade.remove("lista", Qt::CaseInsensitive);
                ypos = ui->labelAnime00Titulo->y()+ui->labelAnime00Titulo->height()+
                        ((ui->labelAnime00Progresso->height())*2)+3;
            }
            else{
                ypos = ui->labelAnime00Titulo->y();
            }
            if(vposicaoGrade.toInt()%3 == 0){
                thisWid->move(ui->labelAnime00Titulo->x(),
                    static_cast<int>(ypos+((2+ui->imagemAnime00->height())*(vposicaoGrade.toInt()/3))));
            }
            else if((vposicaoGrade.toInt()-1)%3 == 0)
                thisWid->move(ui->labelAnime00Titulo->x()
                    +ui->labelAnime00Titulo->width()+13
                    +ui->imagemAnime00->width(),
                    static_cast<int>(ypos+((2+ui->imagemAnime00->height())*(vposicaoGrade.toInt()/3))));
            else if((vposicaoGrade.toInt()-2)%3 == 0)
                thisWid->move(ui->labelAnime00Titulo->x()
                    +((ui->labelAnime00Titulo->width()+13
                    +ui->imagemAnime00->width())*2),
                    static_cast<int>(ypos+((2+ui->imagemAnime00->height())*
                                           (vposicaoGrade.toInt()/3))));
        }
        //Muda as informações do anime
        //Tenho que mudar manualmente mas isso quebra o galho por enquanto
//        else if(this->findChild<QWidget *>(wid->objectName())->objectName().contains("labelInfo", Qt::CaseInsensitive)){
        else if(!thisWid->objectName().contains("imagemanimegrande", Qt::CaseInsensitive)){
            thisWid->resize(
                        static_cast<int>(comprimentoJanela*thisWid->width()),
                        static_cast<int>(alturaJanela*thisWid->height()));
            thisWid->move(
                        static_cast<int>((static_cast<double>(thisWid->x())/1920)*width),
                        static_cast<int>((static_cast<double>(thisWid->y())/821)*widheight));
            if(thisWid->objectName().contains("labelDireitoFundoControles", Qt::CaseInsensitive) &&
                    (thisWid->y() == ui->labelDireitoFundoSinopse->y()+ui->labelDireitoFundoSinopse->height())){
                    thisWid->resize(thisWid->width(),thisWid->height()-1);
                    thisWid->move(thisWid->x(),thisWid->y()+1);
            }
        }
    }
    if(width <= 1366){
        vtituloNormalMedio = "background: transparent; font: 75 5pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);";
        vtituloNormalPequeno = "background: transparent; font: 75 4pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);";
        vfonteNormal = "background: transparent; font: 75 6pt \"Calibri\"; font-weight: bold; color: rgb(20, 20, 20);";
        vfonteNormalNota = "background: transparent; font: 75 6pt \"Calibri\"; font-weight: bold; color: rgb(53, 68, 120);";
        vfonteNormalAnimeEncontrado = "background: transparent; font: 75 6pt \"Calibri\"; font-weight: bold; color: rgb(53, 120, 56);";
        vfonteNormalAnimeNaoComecou = "background: transparent; font: 75 6pt \"Calibri\"; font-weight: bold; color: rgb(120, 60, 53);";
        fontSizeTitulo = "8";
        fontSizeTituloAlternativo = "8";
        fontSizeStatus = "10";
        fontSizeInfo = "9";
        fontSizeMensagem = "7";
        fatualizaFontes();
        //Atualizar resolução da janela de torrents e da janela de configurações
        jtorrent.fmudaResolucao();
        jconfig.fmudaResolucao();
    }
}

void MainWindow::on_botaoAnime_clicked()
{
    vJanelaManga = false;
    vJanelaNovel = false;
    vlistaAtual = "watching";
    vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, vlistaAtual);
    ui->Watching->setText("Watching");
    ui->Completed->setText("Completed");
    ui->OnHold->setText("On Hold");
    ui->Dropped->setText("Dropped");
    ui->PlanToWatch->setText("Plan to Watch");
    ui->PlanToWatch->show();
    vtipoAtual = "anime";
    ui->boxMudarPraLista->setItemText(0, "Watching");
    ui->boxMudarPraLista->setItemText(4, "Plan to Watch");
    ui->botaoDownloadListImages->hide();
    on_botaoHome_clicked();
    on_Watching_clicked();
}

void MainWindow::on_botaoManga_clicked()
{
    vJanelaManga = true;
    vJanelaNovel = false;
    vlistaAtual = "mangareading";
    vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, vlistaAtual);
    ui->Watching->setText("Reading");
    ui->Completed->setText("Completed");
    ui->OnHold->setText("On Hold");
    ui->Dropped->setText("Dropped");
    ui->PlanToWatch->setText("Plan to Read");
    ui->PlanToWatch->show();
    vtipoAtual = "manga";
    ui->boxMudarPraLista->setItemText(0, "Reading");
    ui->boxMudarPraLista->setItemText(4, "Plan to Read");
    ui->botaoDownloadListImages->hide();
    on_botaoHome_clicked();
    on_Watching_clicked();
}

void MainWindow::on_botaoLN_clicked()
{
    vJanelaManga = false;
    vJanelaNovel = true;
    vlistaAtual = "novelreading";
    vlistaSelecionada = cleitorListaAnimes->sortLista(vordem, vlistaAtual);
    ui->Watching->setText("Reading");
    ui->Completed->setText("Completed");
    ui->OnHold->setText("On Hold");
    ui->Dropped->setText("Dropped");
    ui->PlanToWatch->setText("Plan to Read");
    ui->PlanToWatch->show();
    vtipoAtual = "novel";
    ui->boxMudarPraLista->setItemText(0, "Reading");
    ui->boxMudarPraLista->setItemText(4, "Plan to Read");
    ui->botaoDownloadListImages->hide();
    on_botaoHome_clicked();
    on_Watching_clicked();
}

void MainWindow::on_botaoDownloadAnime_clicked()
{
    jtorrent.fprocuraAnimeEspecifico(vlistaSelecionada[vanimeSelecionado]->vnome);
    ui->janelaRotativa->setCurrentIndex(2);
}

void MainWindow::on_botaoAnimeAssistindo_clicked()
{
    ui->barraBusca->setPlainText(cleitorListaAnimes->fbuscaAnimePorIDERetornaTitulo(idAnimeAssistindo));
    on_botaoBusca_clicked();
    ui->barraBusca->clear();
}

void MainWindow::on_botaoSelecionarPastaAnime_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(dir != ""){
        cconfUsuario->fselecionaPastaEspecificaAnime(vlistaSelecionada[vanimeSelecionado]->vid, dir);
    }
}

void MainWindow::on_botaoSeason_clicked()
{
    vanoBuscaAnimes = ui->boxSelecionaAno->currentText().toInt();
    vJanelaSeason = true;
    vJanelaManga = false;
    vJanelaNovel = false;
    vlistaAtual = "season"+QString::number(vanoBuscaAnimes);
    vlistaSelecionada = cleitorListaAnimes->sortLista(vordem,vlistaAtual);
    if(vlistaSelecionada.isEmpty())
        fupdateMessage("The list isn't loaded yet.");
    else
        fupdateMessage("The images from animes that are not in some list are not downloaded by default.\n"
                                   "If needed, you can use the Load button to download and load the images from the "
                                   "animes in this year.");
    ui->NumPagina->setText(QString::number(QDate::currentDate().year()) + " - " + QString::number(vlistaSelecionada.size())
                           + " animes in the list - Page "+QString::number(vpagina)+"/"+
                           QString::number(((vlistaSelecionada.size()-1)/12)+1));
    ui->Watching->setText("Winter");
    ui->Completed->setText("Spring");
    ui->OnHold->setText("Summer");
    ui->Dropped->setText("Fall");
    ui->PlanToWatch->hide();
    vtipoAtual = "anime";
    ui->boxMudarPraLista->setItemText(0, "Watching");
    ui->boxMudarPraLista->setItemText(4, "Plan to Watch");
    vanimeSelecionado = 0;
    vpagina = 1;
    ui->botaoDownloadListImages->show();
    ui->boxSelecionaAno->show();
    finfoAnimeSelecionado();
}

void MainWindow::on_botaoDownloadListImages_clicked()
{
    vcontadorBotaoLoadImages++;
    fsetDownloadImagensAnimesPorAno();
    if(vcontadorBotaoLoadImages != 2)
        QTimer::singleShot(5000, this, SLOT(on_botaoDownloadListImages_clicked()));
    else if(vcontadorBotaoLoadImages == 2)
        vcontadorBotaoLoadImages = 0;
}

void MainWindow::on_botaoAddAlternativeTitle_clicked()
{
    if(!ui->barraAddNomeAlternativo->toPlainText().isEmpty()){
        QStringList tempAnimeList;
        tempAnimeList.append(ui->barraAddNomeAlternativo->toPlainText());
        if(!vcustomNomesAlternativos.contains(vlistaSelecionada[vanimeSelecionado]->vid))
            vcustomNomesAlternativos.insert(vlistaSelecionada[vanimeSelecionado]->vid, tempAnimeList);
        else
            vcustomNomesAlternativos[vlistaSelecionada[vanimeSelecionado]->vid].append((ui->barraAddNomeAlternativo->toPlainText()));
        cleitorListaAnimes->finsereNomeAlternativo(vlistaSelecionada[vanimeSelecionado]->vid,tempAnimeList);
        ui->barraAddNomeAlternativo->clear();
    }
}

void MainWindow::on_botaoAjuda_clicked()
{
    ui->janelaRotativa->setCurrentIndex(3);
}

void MainWindow::on_botaoAnimeAssistindoCerto_clicked()
{
    if(!vnomeAnimeAberto.isEmpty()){
        QStringList tempAnimeList;
        tempAnimeList.append(vnomeAnimeAberto);
        if(!vcustomNomesAlternativos.contains(vlistaSelecionada[vanimeSelecionado]->vid))
            vcustomNomesAlternativos.insert(vlistaSelecionada[vanimeSelecionado]->vid, tempAnimeList);
        else
            vcustomNomesAlternativos[vlistaSelecionada[vanimeSelecionado]->vid].append(vnomeAnimeAberto);
        cleitorListaAnimes->finsereNomeAlternativo(vlistaSelecionada[vanimeSelecionado]->vid,tempAnimeList);
        ui->barraAddNomeAlternativo->clear();
    }
}

void MainWindow::on_botaoDownloadNovaImagem_clicked()
{
    QPointer<QFile> mediumFile(new QFile);
    QPointer<QFile> bigFile(new QFile);

    mediumFile->setFileName(dirMedio+vlistaSelecionada[vanimeSelecionado]->vid+".jpg");
    bigFile->setFileName(dirGrande+vlistaSelecionada[vanimeSelecionado]->vid+".jpg");

    if(mediumFile->exists())
        mediumFile->remove();
    if(bigFile->exists())
        bigFile->remove();
    on_botaoRefresh_clicked();
}
