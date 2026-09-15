#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QDebug>
#include <algorithm>
#include <QDir>
#include <QFuture> //Importante para baixar as imagens no background
#include <QtConcurrent> //E rodar a função em uma thread separada
#include <QTimer> //Para atualizar a lista automaticamente
#include <QDesktopServices> //Abre sites
#include <QDesktopWidget> //Pra resolução
#include <QScreen> //Resolução também
#include <QtMath> //Também resolução
#include <QMutex> //Pra lidar com os mapas nas threads
#include <QMutexLocker> //Pra lidar com os mapas nas threads
#include <QThread>

#include "leitorlistaanimes.h"
#include "confbase.h"
#include "filedownloader.h"
#include "arquivos.h"
#include "confusuario.h"
#include "logger.h"
#include "anilist.h"
#include "robotlib/Robot.h"
#include "anitomy/anitomy.h"
#include "checainfoplayer.h"

#include "janeladeconfig.h"
#include "janelatorrent.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool fcarregaImagensBackground(const QString &lista);
    bool fcarregaImagensSelecionadasBackground();
    void fcarregaImagensLista();
    void finfoAnimeSelecionado();
    void fatualizaRefreshTimer();
    void fatualizaAnilist();

    void fbloqueiaSinaisBotoes();
    void fliberaSinaisBotoes();
    void frefreshListas(bool);
    void fmandaDiretoriosArquivos();
    void favisoErro(const QString &rerro);
    void fmudaResolucao();
    void fcarregouListaSucesso(bool);
    void fcarregouListaFalha();

signals:
    void sterminouCarregarImagens();
    void sanimeReconhecido(QString, QString, QString);
private slots:
    void on_botaoAnime00_clicked();
    void on_botaoAnime01_clicked();
    void on_botaoAnime02_clicked();
    void on_botaoAnime03_clicked();
    void on_botaoAnime04_clicked();
    void on_botaoAnime05_clicked();
    void on_botaoAnime06_clicked();
    void on_botaoAnime07_clicked();
    void on_botaoAnime08_clicked();
    void on_botaoAnime09_clicked();
    void on_botaoAnime10_clicked();
    void on_botaoAnime11_clicked();

    void on_botaoProximaPagina_clicked();
    void on_botaoPaginaAnterior_clicked();

    void on_Watching_clicked();
    void on_Completed_clicked();
    void on_OnHold_clicked();
    void on_Dropped_clicked();
    void on_PlanToWatch_clicked();

    void on_botaoProximoEpisodio_clicked();
    void on_botaoAbrePasta_clicked();
    void on_botaoBusca_clicked();
    void on_botaoRefresh_clicked();
    void on_botaoHome_clicked();
    void on_botaoConfiguracao_clicked();
    void on_botaoNotaMais_clicked();
    void on_botaoNotaMenos_clicked();
    void on_botaoProgressoMais_clicked();
    void on_botaoProgressoMenos_clicked();
    void on_botaoMudarPraLista_clicked();
    void on_botaoAnilist_clicked();
    void on_botaoCrunchyroll_clicked();
    void on_botaoTorrent_clicked();
    void on_botaoRemoverdaLista_clicked();
    void on_botaoOrdemAlfabetica_clicked();
    void on_botaoOrdemProgresso_clicked();
    void on_botaoOrdemScore_clicked();
    void on_botaoOrdemRelease_clicked();
    void on_botaoOrdemType_clicked();
    void on_botaoAnime_clicked();
    void on_botaoManga_clicked();
    void on_botaoLN_clicked();
    void on_botaoDownloadAnime_clicked();
    void on_botaoAnimeAssistindo_clicked();
    void on_botaoSelecionarPastaAnime_clicked();
    void on_botaoSeason_clicked();
    void on_botaoDownloadListImages_clicked();
    void on_botaoAddAlternativeTitle_clicked();
    void on_botaoAjuda_clicked();
    void on_botaoDownloadNovaImagem_clicked();
    void on_botaoAnimeAssistindoCerto_clicked();

    void fretryAnilist();
    void fcarregouListaTeste(bool);
    void fsetIdAdicionado(const QString &lid);
    void fsetIdBaixado(const QString &lid);
    void fsetIdBaixadoGrande(const QString &lid);
    void fInfoAnimeTorrent(const QString &lnomeAnime);
    void fVerificaAnimeAberto();
    void fMostraAnimeAberto(QString, const QString &rnomeAnime, const QString &repisodioAnime);
    void fAumentaProgressoID(const QString &ridAnime, const QString &episodioAnime);
    void fgetMediaPlayersFromList();
    void fgetStreamLanguages();
    void fgetConfigurations();
    void fsetDownloadImagensAnimesPorAno();
    void fsalvaNomesAlternativos();
    void fleNomesAlternativos();
    void fresetaRequestCounter();
    void fatualizaFontes();
    void fupdateMessage(const QString &mensagem);

private:
    Ui::MainWindow *ui;

    leitorlistaanimes *cleitorListaAnimes;
    filedownloader *cfiledownloader;
    arquivos *carquivos;
    anilist *canilist;

    //A classe já está deletando todos os ponteiros. Tentar deletar isso vai ser tentar deletar uma lista vaiza e possivelemnte irá
    //Crashar o programa
    QVector<anime*> vlistaSelecionada;
    QMap<QString,bool> vimagemBaixada;
    QMap<QString,bool> vimagemCarregada;
    QMap<QString,bool> vimagemBaixadaGrande;
    QMap<QString,bool> vimagemCarregadaGrande;

    QVector<int> vdownloadImagensAnos;

    QPointer<confBase> cconfBase;
    QPointer<confUsuario> cconfUsuario;

    //Thread que vai carregar as imagens do anime. Como essa thread fica dentro da própria classe, temos que usar QFuture
    QFuture<void> vcarregaImagens;

    //Variáveis globais
    int vanimeSelecionado;
    int vpagina;
    int vtimerSegundos;
    int vcontadorAssistindoEpisodio;
    int vanoBuscaAnimes;
    int vprogressoLoadingBar;
    int vtotalAnimesLoadingBar;
    int vcontadorBotaoLoadImages;
    int vrateLimitRequestsAnilist;
    QString idAnimeAssistindo;
    QString vordem;
    QString vlistaAtual;
    QString vtipoAtual;
    QString dirGrande;
    QString dirMedio;
    QString dirPequeno;
    QString vnomeAnimeAberto;
    QString vtituloNormalMedio;
    QString vtituloNormalPequeno;
    QString vfonteNormal;
    QString vfonteNormalNota;
    QString vfonteNormalAnimeEncontrado;
    QString vfonteNormalAnimeNaoComecou;
    QString vfonteNormalPagina;
    QString vfonteTitulo;
    QString vfonteSubtitulo;
    QString vfonteStatus;
    QString vfonteInfo;
    QString vfonteMensagem;
    QString vfonteMensagemAnime;
    QString fontName;
    QString fontSizeTitulo;
    QString fontSizeTituloAlternativo;
    QString fontSizeStatus;
    QString fontSizeInfo;
    QString fontSizeMensagem;
    QString fontSizeMensagemAnime;
    QStringList vPlayers;
    QStringList vStreamLanguages;
    QStringList vhistoricoAnilist;
    bool vrefreshAcontecendo = false;
    bool vlistaLidaSucesso = false;
    bool vlistaBaixada = false;
    bool vdownloadImagensMedias = false;
    bool vdownloadImagensGrandes = false;
    bool vdownloadImagensPequenas = false;
    bool vdownloadImagensAno = false;
    bool vdownloadImagensAcabou = true;
    bool vJanelaManga = false;
    bool vJanelaNovel = false;
    bool vJanelaSeason = false;
    bool vbaixandoImagensAno = false;
    bool vbaixaECarregaAnos = false;
    double compHeight;
    double compWidth;

    //Cofigurações
    bool vusarImagensBaixaQualidade;

    QTimer *timer;
    QTimer *tryTimer;
    QTimer *timerRefresh;
    QTimer *timerTorrent;
    QTimer *timerChecaAnimes;
    QTimer *timerChecaDownloadPorAno;
    QTimer timerMaxAnilistRequest;

    QMap<QStringList, QString> vlistaAcoes;
    QMap<QString,QStringList> vcustomNomesAlternativos;
    QMutex vmutex;
    //Thread que vai baixar a lista de anime e busca as pastas. Como essa thread fica fora da classe, temos que usar QThread
    QThread cThread;
    QThread dThread;

    //Janelas
    janeladeconfig jconfig;
    janelatorrent jtorrent;
//    TorrentTab ttorrent;
};
#endif // MAINWINDOW_H
