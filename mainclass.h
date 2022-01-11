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
#include "src/base/Media/mediasearchmanager.h"
#include "src/utilities/Interfaces/IMediaSearchManager.h"
#include "src/base/Media/medialoader.h"
#include "src/utilities/Media/mediacomparer.h"
#include "src/DTO/MediaList.h"
#include "src/utilities/Media/mediadownloader.h"

#include "src/base/Download/downloadqueue.h"
#include "src/base/Download/downloadenums.h"
#include "src/base/MediaType/Anime/animelistmanager.h"
#include "src/utilities/Enums.h"
#include "src/clients/clientmanager.h"
#include "src/base/Media/medialistmanager.h"
#include "src/DTO/ListOrder.h"
#include "src/DTO/CurrentMediaPlaying.h"
#include "src/base/Media/mediamanager.h"
#include "src/base/Media/mediasearchmanager.h"
#include "src/utilities/Media/mediautil.h"
#include "src/utilities/MediaPlayer/mediaplayer.h"
#include "src/base/FileManager/filemanagerloader.h"
#include "src/base/Directory/directoriesconfigurationloader.h"
#include "src/controllers/mediainformationcontroller.h"
#include "src/controllers/mediadatacontroller.h"
#include "src/controllers/medialistcontroller.h"
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
    Q_DECL_UNUSED QVariant fretornaNomeUsuario();
    Q_DECL_UNUSED void fselecionaTipoSeason(QVariant);

    //Falta terminar de refazer
    Q_DECL_DEPRECATED void fbotaoBusca(QVariant); //buttonSearch

    //Novos
    //MediaGridController
    void getMediaList(QVariant order = "Title", QVariant year = 0, bool changeOrder = false);
    void getSelectedMediaData(QVariant selectedMediaGridIndex);
    QVariant getMediaJsonObjectByGridIndex(QVariant gridIndex);
    //MediaListController
    void selectListCurrent();
    void selectListCompleted();
    void selectListPaused();
    void selectListDropped();
    void selectListPlanning();
    void selectTypeAnime();
    void selectTypeManga();
    void selectTypeNovel();
    void selectListSeason(QVariant data);
//    void setMedia();
    //MediaDataController
    void playNextEpisode();
    void openMediaWebpage(QVariant data);
    void openMediaFolder();
    void setMediaProgress(int mediaId, int mediaProgress);
    void setMediaCustomName(QVariant data);
    void removeMediaFromList();
    void setMediaList(QVariant data);

    //GUIController
    void buttonSetMediaProgress(QVariant data);
    void buttonSetMediaScore(QVariant data);
    void buttonNextPage();
    void buttonLastPage();
    void buttonMenuMedia();
    void buttonMenuConfigurations();
    void buttonMenuTorrent();
    void buttonSearch(QVariant data);
    void buttonSetMediaFolder(QVariant);


    QVariant getUsername();
    QVariant getUserAvatar();
    void refreshMediaList();
    void getCurrentMediaPlaying();
    void clientConnection(bool connected);
    void emitSignalIdMedia(int listMediaInde, bool nullSignal = false);
//    void emitSignalIdMedia();
    void emitSignalSelectedMedia(QJsonObject selectedMediaObject);

signals:
    //Não feitos ainda
    void sdirImagensGrandes(QVariant data);
    void sdirImagensMedias(QVariant data);
    void sdirImagensPequenas(QVariant data);

    void sconnectGUI(QVariant data);
    void stimer(QVariant data);
    void sconfigSelecionada(QVariant data);
    void storrentPronto();
    void sbaixouImagensMedias();


    //Novos
    void signalMediaGrid(QVariant gridId, QVariant mediaData);
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
    void connectFail();
    void loadMediaList();
    void setUpdateTimer();
    void updateTimer();
    void setDownloads();
    void setObjects();

    //Classes
    Database *cdatabase;
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
    QPointer<MediaController> mediaController;
    QPointer<MediaDirectories> mediaDirectories;
    QPointer<FileManagerLoader> fileManagerLoader;
    QPointer<MediaInformationController> mediaInformationController;
    QPointer<MediaListController> mediaListController;
    QPointer<MediaDataController> mediaDataController;
//    QVector<Media*> activeMediaList;

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
    bool firstConnection = false;
    QTime listUpdateTimer;
    QPointer<QTimer> listUpdateCountdown;

    Enums::mediaOrder mediaOrder;
    Enums::mediaType mediaType;
    Enums::mediaList mediaList;
    Enums::orderType mediaOrderType;
    menu selectedMenu;
};

#endif // MAINCLASS_H
