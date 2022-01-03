#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QDebug>
#include <QVector> //Cria a lista de animes
#include <QFile> //Abre arquivo do anilist
#include <QTextStream> //Lê linhas do arquivo
#include <QPointer> // Crio o ponteiro pro anime
#include <QDateTime> //Data que lança o episódio
#include <QRegExp>

#include "src/utilities/singleton.h"

#include "src/base/anime.h"
#include "src/utilities/formatapalavras.h"
//REFEITO
class Database : public QObject
{
    Q_OBJECT
    static Database *createInstance();

public:
    explicit Database(QObject *parent = nullptr);
    ~Database();
    static Database *instance();
    enum type{ANIME, MANGA, NOVEL, SEASON};
    Q_ENUM(type)
    enum list{CURRENT, COMPLETED, PAUSED, DROPPED, PLANNING, SEARCH, WINTER, SUMMER, FALL, SPRING};
    Q_ENUM(list)
    enum tipoHash{LISTA, POSICAO, NOME, NOMEALTERNATIVO};
    Q_ENUM(tipoHash)

///Classe parente
    bool comparaNomes(QString nomeOficial, QString nomeIngles, QStringList nomesAlternativos, QString nomeBuscado);
    void insereNomesHashNomeAnimesPorId(QString id, QStringList nomes);
///Classe de leitura
    bool freadDatabaseFile();
    bool freadDatabaseFile2();
    bool fchecaDatabaseReady();
    void finsereDataHashId(QString tipo, QString lista, QString id, QStringList nomes);
    void insereDataHashPosicao(type tipo, QString id, int posicao);
    void fcarregaListaAnoEmThread();

    //return functions
    QVector<anime*> returnList(QString lista, enum type tipo, QString argumento = "");
    QVector<anime*> returnAnimeList(QString lista, QString argumento = "");
    QVector<anime*> returnMangaList(QString lista, QString argumento = "");
    QVector<anime*> returnNovelList(QString lista, QString argumento = "");
    QVector<anime*> returnAnimeSeasonalList(QString lista);
    QVector<anime*> returnSortList(const QString &rordem, QString lista, type type);

    QVector<anime*> returnAnimeYearlyList(int ano);
    QVector<anime*> returnSearchList(const QString &rnome, type rtipoMidia);

    ///Classe de busca
    //funções de buscas - OK
    QString fbuscaIDRapido(const QString &rnomeAnime);
    QString fbuscaIDRetornaLista(const QString &ridAnime);
    int fbuscaIDRetornaPosicao(const QString &ridAnime);
    QString fbuscaIDRetornaEpisodio(const QString &rid);
    QString fbuscaIDRetornaNota(const QString &rid);
    QString fbuscaIDRetornaTitulo(const QString &rid);
    QString fbuscaNomeRetornaID(const QString &rnomeAnime);
    anime* fbuscaAnimeNoAno(int, const QString &rid);
    anime* fretornaAnimePorID(const QString &rid);
    anime* fretornaAnimePorPosicao(const QString &rlista, int posicao);

    QHash<QString, int> retornaHash(tipoHash tipoHash, type tipoMidia, int valorInt = 0);
    QHash<QString, QString> retornaHash(tipoHash tipoHash, type tipoMidia, QString valorQString = "");
    QHash<QString, QStringList> retornaHash(tipoHash tipoHash, type tipoMidia, QStringList valorQStringList = QStringList());


    ///Classe de gerenciamento
    //Funções de anime - OK
    bool fmudaLista(const QString &rid, const QString &rlista, type rtipo);
    bool fmudaNota(const QString &rid, const QString &rnota);
    bool fmudaProgresso(const QString &rid, const QString &rprogresso);
    bool fdeletedaLista(const QString &rid);
    bool finsereNomeAlternativo(const QString &rid, const QStringList &rnome);

    //Funções de listas
    void fdeletaListaAnimes();
    void fcarregaIdNomeAno();
    void fsalvaIdNomeAno();
    void fsalvaListaNomesAlternativos();
    void fleListaNomesAlternativos();

    //Todo - Verificar quais funções precisam de mais de um argumento e ajustar o mediaLoader
    ///Classe de limpeza
    //Limpa arquivo
    QString limpaNull(QString linha);
    QString limpaImagem(QString linha);
    QString limpaSinopse(QString linha);
    QString limpaEpisodiosTotais(QString linha);
    QString limpaFormato(QString linha);
    QString limpaId(QString linha);
    QString limpaStreamUrl(QString linha);
    QString limpaChapters(QString linha);
    QString limpaVolumes(QString linha);
    QString limpaDatabaseUrl(QString linha);
    QString limpaDataProximoEpisodio(QString linha);
    QString limpaAiring(QString linha, QDate &ldataEpisodioConvertida, QTime &lhoraLancamentoEpisodio);
    QString limpaNumeroProximoEpisodio(QString linha);
    QString limpaSeason(QString linha);
    QString limpaMonth(QString linha);
    QString limpaYear(QString linha, QString season);
    QString limpaStatus(QString linha, QString &dataEpisodioFinal, QDate &ldataEpisodioConvertida, QTime &lhoraLancamentoEpisodio);
    QString limpaNomesAlternativos(QString linha);
    QString limpaNomeIngles(QString linha);
    QString limpaNomeRomaji(QString linha, int &temporada);
    QString limpaProgresso(QString linha);
    QString limpaScoreMedio(QString linha);
    QString limpaScoreGeral(QString linha);
    QString limpaLista(QString linha);
    QDate setDataEstreia(QString ano, QString mes, QString season);

    ///Classe de gerenciamento
    //Funções de checagem e inserção
    QStringList limpaStringList(QString);
    QString updateStatus(QString);
    bool insereAnimeLista(QStringList, QString);
    bool updateAnimeInfo(QStringList, QString);
    bool updateAnime(QStringList, QString);
    bool updateManga(QStringList, QString);
    bool updateNovel(QStringList, QString);

signals:
    void sAnimeAdicionadoNaLista(QString);
    void sMangaAdicionadoNaLista(QString);
    void sNovelAdicionadoNaLista(QString);


private:
    FormataPalavras formatador;

    bool vdatabaseReady;

    //Listas de Animes
    QVector<anime*> vlistaAnimeWatching;
    QVector<anime*> vlistaAnimeCompleted;
    QVector<anime*> vlistaAnimeOnHold;
    QVector<anime*> vlistaAnimeDropped;
    QVector<anime*> vlistaAnimePlanToWatch;

    //Listas de Mangas
    QVector<anime*> vlistaMangaReading;
    QVector<anime*> vlistaMangaCompleted;
    QVector<anime*> vlistaMangaOnHold;
    QVector<anime*> vlistaMangaDropped;
    QVector<anime*> vlistaMangaPlanToRead;

    //Listas de novels
    QVector<anime*> vlistaNovelReading;
    QVector<anime*> vlistaNovelCompleted;
    QVector<anime*> vlistaNovelOnHold;
    QVector<anime*> vlistaNovelDropped;
    QVector<anime*> vlistaNovelPlanToRead;

    //Listas de Seasons
    QVector<anime*> vlistaAnimeSeasonWinter;
    QVector<anime*> vlistaAnimeSeasonSummer;
    QVector<anime*> vlistaAnimeSeasonFall;
    QVector<anime*> vlistaAnimeSeasonSpring;

    //Listas mistas
    QVector<anime*> vlistaBusca;
    QVector<anime*> vlistaTemp;
    QVector<anime*> vlistaAno;
    QVector<int> vidEntrada;

    //Hashs de busca
    QHash<QString,QString> vhashListaAnimesPorId;
    QHash<QString,QString> vhashListaMangasPorId;
    QHash<QString,QString> vhashListaNovelsPorId;
    QHash<QString,int> vhashPosicaoAnimesPorId;
    QHash<QString,int> vhashPosicaoMangasPorId;
    QHash<QString,int> vhashPosicaoNovelsPorId;
    QHash<QString,QStringList> vhashNomeAnimesPorId;
    QHash<QString,QStringList> vhashNomeMangasPorId;
    QHash<QString,QStringList> vhashNomeNovelsPorId;
    QHash<QString, QStringList> vlistaNomesAlternativos;
    QHash<int,QString> vhashSizeListasPorAno;
};

#endif // DATABASE_H
