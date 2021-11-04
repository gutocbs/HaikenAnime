#ifndef MAINCLASS_H
#define MAINCLASS_H

#include <QObject>
#include <QVector>
#include <QPixmap>
#include <QDesktopServices>
#include <QUrl>
#include <QMetaEnum>
#include <QTimer>
#include <QThread>
#include <QDate>
#include <QTime>
#include <QVariant>
#include <QVariantList>

#include "src/base/database.h"
#include "src/base/anime.h"
#include "src/base/confbase.h"
#include "src/base/arquivos.h"
#include "src/base/confusuario.h"
#include "src/utilities/client.h"
#include "src/utilities/downloader.h"
#include "src/base/abaconfig.h"
#include "src/base/abatorrent.h"
#include "lib/robotlib/Robot.h"
#include "src/utilities/playerrecognition.h"
#include "src/base/Anime/mediasearchmanager.h"
#include "src/utilities/Interfaces/IMediaSearchManager.h"
#include "src/base/Anime/medialoader.h"
#include "src/utilities/Media/mediacomparer.h"
#include "src/DTO/MediaList.h"
#include "src/utilities/Media/mediadownloader.h"

#include "src/base/Download/downloadqueue.h"
#include "src/base/Download/downloadenums.h"
#include "src/base/Anime/animelistmanager.h"
#include "src/utilities/Enums.h"
#include "src/clients/clientmanager.h"
#include "src/base/Media/medialistmanager.h"
#include "src/DTO/ListOrder.h"
#include "src/DTO/CurrentMediaPlaying.h"
#include "src/base/Media/mediamanager.h"
#include "src/base/Anime/mediasearchmanager.h"
#include "src/utilities/Media/mediautil.h"
#include "src/utilities/MediaPlayer/mediaplayer.h"

class MainClass : public QObject
{
    Q_OBJECT
//    enum listas{CURRENT, COMPLETED, PAUSED, DROPPED, PLANNING};
//    enum type{ANIME, MANGA, NOVEL};
//    Q_ENUM(type)
public:
    explicit MainClass(QObject *parent = nullptr);
    ~MainClass();

    enum lista{CURRENT, COMPLETED, PAUSED, DROPPED, PLANNING, WINTER, SUMMER, FALL, SPRING, SEARCH};
    Q_ENUM(lista)
    enum janela{MAIN, TORRENT, CONFIG};
    Q_ENUM(janela)
    enum menu{MEDIA, TORRENTS, CONFIGURATION};
    Q_ENUM(menu)
public slots:

    //TODO - Mudar isso vai foder tudo no qml. Vou ter que olhar bem lá para resolver
    //Organizar aqui pra por todos os gets e sets juntos
    //Na hora de refatorar, deixar aqui só as funções que lidam com a interface. O resto vai pra outra classe
    void fupdateTimer();
    void fselecionaPastaespecificaAnime(QVariant);
    void ftryClientConnection(bool);
    void fconnectFail();
    void fsetauthCode(QVariant);
    void fsetUsername(QVariant);
    void fsetClient(QVariant);
    bool fsetDirectory(QVariant);
    bool fremoveDirectory(QVariant);
    void fsaveConfig();
    void fsetDirConfig(QVariantList);
    QVariant fgetDir();
    void fsetTorrentList();
    QVariant fgetTorrentList(QVariant);
    void fchangeTorrentState(QVariant, QVariant);
    void fsearchAnimeFromTorrent(QVariant);
    QVariant fopenTorrentLink(QVariant);
    void fbotaoDownloadTorrents();

    //Remover, mas verificar no QML antes
    Q_DECL_UNUSED void fordemLista(QVariant); //void getMediaList(QVariant order = "Title");
    Q_DECL_UNUSED void fclientUpdate();
    Q_DECL_UNUSED void fselecionaTipoAnime();
    Q_DECL_UNUSED void fselecionaTipoManga();
    Q_DECL_UNUSED void fselecionaTipoNovel();
    Q_DECL_UNUSED void fselecionaListaCurrent();
    Q_DECL_UNUSED void fselecionaListaCompleted();
    Q_DECL_UNUSED void fselecionaListaPaused();
    Q_DECL_UNUSED void fselecionaListaDropped();
    Q_DECL_UNUSED void fselecionaListaPlanning();
    Q_DECL_UNUSED void fabreProximoEpisodio(); //void playNextEpisode();
    Q_DECL_UNUSED void fmostraListaAnimes(); //void getMediaListPage();
    Q_DECL_UNUSED void finfoAnimeSelecionado(QVariant); //getSelectedMediaData()
    Q_DECL_UNUSED QVariant fretornaNumeroAnos(); //Só remover, mas ver onde uso isso no qml. Acho que é pra pegar o número de anos disponíveis na busca
    Q_DECL_UNUSED QVariant fretornaNomeAnimePosicao(QVariant); //getMediaJsonObjectByGridIndex()
    Q_DECL_UNUSED QVariant fretornaEpisodiosAnimePosicao(QVariant); //Todas as funções vão ser uma só
    Q_DECL_UNUSED QVariant fretornaNotaAnimePosicao(QVariant);//QML deve chamar a mesma, montar o objeto e distribuir as variáveis
    Q_DECL_UNUSED QVariant fretornaListaAnimePosicao(QVariant); //getMediaJsonObjectByGridIndex()
    Q_DECL_UNUSED QVariant fretornaEpisodioAnimeEncontrado(QVariant); //getMediaJsonObjectByGridIndex()
    Q_DECL_UNUSED QVariant fretornaNomeUsuario();
    Q_DECL_UNUSED QVariant fretornaPathAvatar();
    Q_DECL_UNUSED void fabreSite(QVariant);
    Q_DECL_UNUSED void fselecionaTipoSeason(QVariant);
    Q_DECL_UNUSED void fabrePastaAnime(); //openMediaFolder
    Q_DECL_UNUSED void fabreStream(); //removido
    Q_DECL_UNUSED void frefresh(); //refreshMediaList
    Q_DECL_UNUSED void fchecaAnimeAssistido(); //getCurrentMediaPlaying
    Q_DECL_UNUSED void fAumentaProgressoID(const QString &ridAnime, const QString &episodioAnime); //setMediaProgress
    Q_DECL_UNUSED void fmudaProgresso(QVariant); //buttonSetMediaProgress
    Q_DECL_UNUSED void fmudaNota(QVariant); //buttonSetMediaScoreButton
    Q_DECL_UNUSED void fresetRequests(); //removido
    Q_DECL_UNUSED void fadicionaNomeAlternativo(QVariant); //setMediaCustomName
    Q_DECL_UNUSED void fremoveFromList(); //removeMediaFromList
    Q_DECL_UNUSED void fproximaPagina(); //buttonNextPage
    Q_DECL_UNUSED void fanteriorPagina(); //buttonLastPage
    Q_DECL_UNUSED void fmudaListaAnime(QVariant); //setMediaList
    Q_DECL_UNUSED void fbotaoHome();
    Q_DECL_UNUSED void fbotaoConfig();
    Q_DECL_UNUSED void fbotaoTorrent();

    //Falta terminar de refazer
    Q_DECL_DEPRECATED void fconnectSuccess();
    Q_DECL_DEPRECATED void fbotaoBusca(QVariant); //buttonSearch

    //Novos
    void getMediaList(QVariant order = "Title", QVariant year = 0);

    void selectTypeAnime();
    void selectTypeManga();
    void selectTypeNovel();

    void selectListCurrent();
    void selectListCompleted();
    void selectListPaused();
    void selectListDropped();
    void selectListPlanning();
    void playNextEpisode();
    void getMediaListPage();
    void emitSignalIdMedia(int listMediaIndex, bool nullSignal = false);
    void getSelectedMediaData(QVariant selectedMediaGridIndex);
    QVariant getMediaJsonObjectByGridIndex(QVariant gridIndex);
    QVariant getUsername();
    QVariant getUserAvatar();
    void openMediaWebpage(QVariant data);
    void selectListSeason(QVariant data);
    void openMediaFolder();
    void refreshMediaList();
    void getCurrentMediaPlaying();
    void setMediaProgress(int mediaId, int mediaProgress);
    void buttonSetMediaProgress(QVariant data);
    void buttonSetMediaScoreButton(QVariant data);
    void setMediaCustomName(QVariant data);
    void removeMediaFromList();
    void buttonNextPage();
    void buttonLastPage();
    void setMediaList(QVariant data);
    void buttonMenuMedia();
    void buttonMenuConfigurations();
    void buttonMenuTorrent();
    void buttonSearch(QVariant data);

signals:
    //Antigos
    void sidAnime1(QVariant data);
    void sidAnime2(QVariant data);
    void sidAnime3(QVariant data);
    void sidAnime4(QVariant data);
    void sidAnime5(QVariant data);
    void sidAnime6(QVariant data);
    void sidAnime7(QVariant data);
    void sidAnime8(QVariant data);
    void sidAnime9(QVariant data);
    void sidAnime10(QVariant data);
    void sidAnime11(QVariant data);
    void sidAnime12(QVariant data);

    void sidAnimeSelecionado(QVariant data);
    void snomeAnimeSelecionado(QVariant data);
    void snomeAlternativoAnimeSelecionado(QVariant data);
    void ssinopseAnimeSelecionado(QVariant data);
    void sstatusAnimeSelecionado(QVariant data);
    void sseasonAnimeSelecionado(QVariant data);
    void smediaGloballAnimeSelecionado(QVariant data);
    void smediaPessoalAnimeSelecionado(QVariant data);
    void sreleaseAnimeSelecionado(QVariant data);
    void sepisodiosLancadosAnimeSelecionado(QVariant data);
    void sepisodiosAssistidosAnimeSelecionado(QVariant data);
    void sepisodiosTotaisAnimeSelecionado(QVariant data);
    void stipoAnimeSelecionado(QVariant data);
    void sproximoEpisodioAnimeSelecionado(QVariant data);
    void simagemAnimeSelecionado(QVariant data);
    void sanimeReconhecidoID(QVariant dataId, QVariant dataNome, QVariant dataEpisodio);
    //Não feitos ainda
    void sdirImagensGrandes(QVariant data);
    void sdirImagensMedias(QVariant data);
    void sdirImagensPequenas(QVariant data);

    void sconnectGUI(QVariant data);
    void stimer(QVariant data);

    void sbotaoHome();
    void sbotaoConfig();
    void sbotaoTorrent();
    void sconfigSelecionada(QVariant data);
    void storrentPronto();
    void sbaixouImagensMedias();


    //Novos
    void signalIdMediaGrid0(QVariant data);
    void signalIdMediaGrid1(QVariant data);
    void signalIdMediaGrid2(QVariant data);
    void signalIdMediaGrid3(QVariant data);
    void signalIdMediaGrid4(QVariant data);
    void signalIdMediaGrid5(QVariant data);
    void signalIdMediaGrid6(QVariant data);
    void signalIdMediaGrid7(QVariant data);
    void signalIdMediaGrid8(QVariant data);
    void signalIdMediaGrid9(QVariant data);
    void signalIdMediaGrid10(QVariant data);
    void signalIdMediaGrid11(QVariant data);

    void signalSelectedMedia(QVariant data);
    void signalSelectedMediaCover(QVariant data);
    void currentMediaPlayer(QVariant dataId, QVariant dataName, QVariant dataEpisode);

    void signalMenuMedia();
    void signalMenuConfiguration();
    void signalMenuTorrents();
private:
    void fconnections();

    //Connect and update lists
    void connectSuccess();
    void loadMediaList();
    void setUpdateTimer();
    void setDownloads();
    int getPageIndexRange();
    void setObjects();

    //Classes
    Database *cdatabase;
    confBase *cconfiguracoesDiretoriosPadrao;
    arquivos *carquivos;
    confUsuario *cconfiguracoesUsuarioDiretorios;
    Client *cclient;
    Downloader *cdownloader;
    abaConfig *cabaConfig;
    abaTorrent *cabaTorrent;
    QVector<anime*> vlistaSelecionada;

    int vindexAnimeSelecionado;
    int vposicaoGridAnimeSelecionado;
    int vpagina;
    int vanoBuscaAnimes;
    int vcontadorAssistindoEpisodio;
    //Essa variável controla o número de requests por minuto que é possível fazer no site.
    int vrateLimitRequests;

    QString vordemLista;
    QString vlistaAtual;
    Database::type vtipoAtual;
    janela vjanelaAtual;

    QMetaEnum vmetaEnumLista;
    QMetaEnum vmetaEnumTipo;

    QThread tthreadDiretorios;

    QStringList vlistaFilaTipo;
    QStringList vlistaFilaLista;
    QList<int> vlistaFilaSize;
    QMap<QStringList, QString> vlistaAcoes;

    QTimer *vtimerCountdown;
    QTimer *vtimerChecaAssistindo;
    QTimer timerMaxClientRequests;
    QTime time;

    //Novos
    QPointer<MediaDownloader> mediaDownloader;
    QPointer<DownloadQueue> downloadQueue;
    QPointer<IMediaListManager> mediaListManager;
    QPointer<ClientManager> clientManager;
    QPointer<MediaPlayer> mediaPlayer;
    QPointer<IMediaSearchManager> mediaSearchManager;
    QPointer<IMediaManager> mediaManager;
    QVector<Media*> activeMediaList;

    //TODO - Novas classes de controle
    //Classe SelectedMedia com dados:
    //Media*
    //SelectedMediaIndex
//    int selectedMediaGridIndex;

    //Classe SelectedMediaList
    //activeMediaList
    //Enums::mediaOrder mediaOrder;
//    Enums::mediaType mediaType;
//    Enums::mediaList mediaList;
//    Enums::orderType mediaOrderType;
//    int selectedPage;

    int selectedMediaIndex;
    int selectedPage;
    int selectedMediaGridIndex;
    int currentMediaPlayingCounter;
    QTime listUpdateTimer;
    QPointer<QTimer> listUpdateCountdown;

    Enums::mediaOrder mediaOrder;
    Enums::mediaType mediaType;
    Enums::mediaList mediaList;
    Enums::orderType mediaOrderType;
    menu selectedMenu;
};

#endif // MAINCLASS_H
