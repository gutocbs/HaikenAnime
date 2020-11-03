#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QDebug>
#include <QVector> //Cria a lista de animes
#include <QFile> //Abre arquivo do anilist
#include <QTextStream> //Lê linhas do arquivo
#include <QPointer> // Crio o ponteiro pro anime
#include <QDateTime> //Data que lança o episódio

#include "src/utilities/singleton.h"

#include "src/base/anime.h"
#include "src/utilities/formatapalavras.h"

class Database : public QObject
{
    Q_OBJECT
    static Database *createInstance();

public:
    explicit Database(QObject *parent = nullptr);
    static Database *instance();
    enum type{ANIME, MANGA, NOVEL, SEASON};
    Q_ENUM(type)

    bool freadDatabaseFile();
    bool fchecaDatabaseReady();
    void finsereDataHashId(QString tipo, QString lista, QString id, QStringList nomes);
    void fcarregaListaAnoEmThread();

    //return functions
    QVector<anime*> returnAnimeList(QString list);
    QVector<anime*> returnAnimeYearlyList(int ano);
    QVector<anime*> returnAnimeSeasonalList(QString list);
    QVector<anime*> returnMangaList(QString list);
    QVector<anime*> returnNovelList(QString list);
    QVector<anime*> returnSearchList(const QString &rnome, type rtipoMidia);
    QVector<anime*> returnSortList(const QString &rordem, QString rlista, type type);

    //funções de buscas
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

    //Funções de anime
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
    QVector<anime*> llistaTemp;

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
