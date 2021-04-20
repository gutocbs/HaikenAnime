#ifndef DATABASESEARCH_H
#define DATABASESEARCH_H

#include <QObject>
#include <QVector> //Cria a lista de animes
#include <QPointer>
#include "src/base/anime.h"
#include "src/base/database.h"
#include "src/utilities/formatapalavras.h"

class DatabaseSearch : public Database
{
    Q_OBJECT
public:
    explicit DatabaseSearch(QObject *parent = nullptr);
    void loadListName();

//    QVector<anime*> returnList(QString lista, enum Database::type tipo);
    int buscaIDRetornaPosicao(const QString &ridAnime, enum Database::type tipo);
    QString buscaIDRapido(const QString &rnomeAnime);
    QString buscaIDRetornaLista(const QString &ridAnime, enum Database::type tipo);
    QString buscaIDRetornaEpisodio(const QString &rid, enum Database::type tipo);
    QString IDRetornaNota(const QString &rid, enum Database::type tipo);
    QString IDRetornaTitulo(const QString &rid, enum Database::type tipo);
    QString buscaNomeRetornaID(const QString &rid, enum Database::type tipo);
    anime* fbuscaAnimeNoAno(int, const QString &rid);
    anime* fretornaAnimePorID(const QString &rid);
    anime* fretornaAnimePorPosicao(const QString &rlista, int posicao);
signals:

private:
    int retornaPosicao(const QString &ridAnime, Database::type);
    QStringList nomeListas;
//    QPointer<Database> cdatabase;
};

#endif // DATABASESEARCH_H
