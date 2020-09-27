#ifndef LEITORLISTAANIMES_H
#define LEITORLISTAANIMES_H

#include <QObject>
#include <QDebug>
#include <QVector> //Cria a lista de animes
#include <QFile> //Abre arquivo do anilist/taiga para leitura
#include <QTextStream> //Lê linhas do arquivo
#include <QPointer> // Crio o ponteiro pro anime
#include <QDateTime> //Data que lança o episódio
#include <QHash> //Pra procurar o anime com rapidez
#include <QDir>

#include "src/utilities/singleton.h"

#include "src/base/anime.h"
#include "src/utilities/formatapalavras.h"

class leitorlistaanimes : public QObject
{
    Q_OBJECT
    static leitorlistaanimes *createInstance();

public:
    explicit leitorlistaanimes(QObject *parent = nullptr);
    ~leitorlistaanimes();
    static leitorlistaanimes* instance();
    enum type{ANIME, MANGA, NOVEL, SEASON};
    Q_ENUM(type)

    bool fleJson();
    bool fmudaLista(const QString &rid, const QString &rlista, type rtipo);
    bool fmudaNota(const QString &rid, const QString &rnota);
    bool fmudaProgresso(const QString &rid, const QString &rprogresso);
    bool fdeletedaLista(const QString &rid);
    void fdeletaListaAnimes();
    void finsereNomeAlternativo(const QString &rid,const QStringList &rlistaNomesAlternativos);
    void fleListaIdsNomesAnos();
    void fsalvaListaIdsNomesAnos();

    int fbuscaAnimePorIDERetornaPosicao(const QString &ridAnime);
    QString fbuscaAnimePorIDERetornaEpisodio(const QString &rid);
    QString fbuscaAnimePorIDERetornaNota(const QString &rid);
    QString fbuscaAnimePorIDERetornaTitulo(const QString &rid);
    QString fbuscaAnimePorIDERetornaLista(const QString &ridAnime);
    QString fprocuraIdNasListasRapido(const QString &rnomeAnime);
    QString fprocuraAnimeNasListas(const QString &rnomeAnime);

    QVector<anime*> retornaListaWatching();
    QVector<anime*> retornaListaCompleted();
    QVector<anime*> retornaListaOnHold();
    QVector<anime*> retornaListaDropped();
    QVector<anime*> retornaListaPlanToWatch();
    QVector<anime*> retornaListaMangaReading();
    QVector<anime*> retornaListaMangaCompleted();
    QVector<anime*> retornaListaMangaOnHold();
    QVector<anime*> retornaListaMangaDropped();
    QVector<anime*> retornaListaMangaPlanToRead();
    QVector<anime*> retornaListaNovelReading();
    QVector<anime*> retornaListaNovelCompleted();
    QVector<anime*> retornaListaNovelOnHold();
    QVector<anime*> retornaListaNovelDropped();
    QVector<anime*> retornaListaNovelPlanToRead();
    QVector<anime*> retornaListaAnimeWinter();
    QVector<anime*> retornaListaAnimeSpring();
    QVector<anime*> retornaListaAnimeSummer();
    QVector<anime*> retornaListaAnimeFall();
    QVector<anime*> retornaListaBusca();
    QVector<anime*> sortLista(const QString &rordem, QString rlistas, type type);
    QVector<anime*> fbuscaLista(const QString &rnome, type rtipoMidia);
    QVector<anime*> fleListaAno(int);
    anime* fbuscaAnimeNoAno(int, const QString &rid);
    anime *fretornaAnimePorID(const QString &rid);
    void fcarregaListaAnoEmThread();

signals:
    void sAnimeAdicionadoNaLista(QString);
    void sMangaAdicionadoNaLista(QString);
    void sNovelAdicionadoNaLista(QString);

private slots:

private:
    FormataPalavras formatador;

    QVector<anime*> vlistaWatching;
    QVector<anime*> vlistaCompleted;
    QVector<anime*> vlistaOnHold;
    QVector<anime*> vlistaDropped;
    QVector<anime*> vlistaPlanToWatch;
    QVector<anime*> vlistaBusca;
    QVector<anime*> llistaTemp;
    QVector<anime*> vlistaMangaReading;
    QVector<anime*> vlistaMangaCompleted;
    QVector<anime*> vlistaMangaOnHold;
    QVector<anime*> vlistaMangaDropped;
    QVector<anime*> vlistaMangaPlanToRead;
    QVector<anime*> vlistaNovelReading;
    QVector<anime*> vlistaNovelCompleted;
    QVector<anime*> vlistaNovelOnHold;
    QVector<anime*> vlistaNovelDropped;
    QVector<anime*> vlistaNovelPlanToRead;
    QVector<anime*> vlistaAnimeSeasonWinter;
    QVector<anime*> vlistaAnimeSeasonSummer;
    QVector<anime*> vlistaAnimeSeasonFall;
    QVector<anime*> vlistaAnimeSeasonSpring;
    QHash<QString,QString> vHashListaAnimesPorId;
    QHash<QString,QString> vHashListaMangasPorId;
    QHash<QString,QString> vHashListaNovelsPorId;
    QHash<QString,int> vHashPosicaoAnimesPorId;
    QHash<QString,int> vHashPosicaoMangasPorId;
    QHash<QString,int> vHashPosicaoNovelsPorId;
    QHash<QString,QStringList> vHashNomeAnimesPorId;
    QHash<QString,QStringList> vHashNomeMangasPorId;
    QHash<QString,QStringList> vHashNomeNovelsPorId;
    QHash<int,QString> vHashSizeListasPorAno;
};

#endif // LEITORLISTAANIMES_H
