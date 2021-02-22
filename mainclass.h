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
public slots:

    //Organizar aqui pra por todos os gets e sets juntos
    void fmostraListaAnimes();
    void finfoAnimeSelecionado(QVariant);
    QVariant fretornaNumeroAnos();
    QVariant fretornaNomeAnimePosicao(QVariant);
    QVariant fretornaEpisodiosAnimePosicao(QVariant);
    QVariant fretornaNotaAnimePosicao(QVariant);
    QVariant fretornaListaAnimePosicao(QVariant);
    QVariant fretornaEpisodioAnimeEncontrado(QVariant);
    QVariant fretornaNomeUsuario();
    QVariant fretornaPathAvatar();
    void fupdateTimer();

    void fordemLista(QVariant);
    void fabreSite(QVariant);
    void fselecionaTipoAnime();
    void fselecionaTipoManga();
    void fselecionaTipoNovel();
    void fselecionaTipoSeason(QVariant);
    void fselecionaListaCurrent();
    void fselecionaListaCompleted();
    void fselecionaListaPaused();
    void fselecionaListaDropped();
    void fselecionaListaPlanning();
    void fabreProximoEpisodio();
    void fabrePastaAnime();
    void fabreStream();
    void frefresh();
    void fchecaAnimeAssistido();
    void fAumentaProgressoID(const QString &ridAnime, const QString &episodioAnime);
    void fmudaProgresso(QVariant);
    void fmudaNota(QVariant);
    void fresetRequests();
    void fadicionaNomeAlternativo(QVariant);
    void fselecionaPastaespecificaAnime(QVariant);
    void fremoveFromList();

    void fproximaPagina();
    void fanteriorPagina();

    void fmudaListaAnime(QVariant);
    void ftryClientConnection(bool);
    void fconnectSuccess();
    void fconnectFail();
    void fclientUpdate();

    void fbotaoHome();
    void fbotaoConfig();
    void fbotaoTorrent();
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
    void fbotaoBusca(QVariant);
    void fbotaoDownloadTorrents();

signals:
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

    void sdirImagensGrandes(QVariant data);
    void sdirImagensMedias(QVariant data);

    void sconnectGUI(QVariant data);
    void stimer(QVariant data);

    void sbotaoHome();
    void sbotaoConfig();
    void sbotaoTorrent();
    void sconfigSelecionada(QVariant data);
    void storrentPronto();
    void sbaixouImagensMedias();

    void sanimeReconhecidoID(QVariant dataId, QVariant dataNome, QVariant dataEpisodio);


private:
    void fconnections();
    void fdownloadCoverImages();

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
};

#endif // MAINCLASS_H
