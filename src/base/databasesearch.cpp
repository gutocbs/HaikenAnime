#include "databasesearch.h"

DatabaseSearch::DatabaseSearch(QObject *parent) : Database(parent)
{
//    cdatabase = new Database();
//    cdatabase = instance();
    loadListName();
}

void DatabaseSearch::loadListName()
{
    nomeListas.append("CURRENT");
    nomeListas.append("COMPLETED");
    nomeListas.append("PLANNING");
    nomeListas.append("DROPPED");
    nomeListas.append("PAUSED");
}

//QVector<anime *> DatabaseSearch::returnList(QString lista, Database::type tipo)
//{
//    return returnList(lista, tipo);
//}

QString DatabaseSearch::buscaIDRapido(const QString &rnomeAnime)
{
    QHash<QString, QString> hashBuscada = retornaHash(Database::NOME,Database::ANIME, "");
    foreach(QString key, hashBuscada.keys()){
        if(hashBuscada[key].contains(rnomeAnime))
            return key;
    }
    return "";
}

QString DatabaseSearch::buscaIDRetornaLista(const QString &ridAnime, Database::type tipo)
{
    QHash<QString, QString> hashBuscada = retornaHash(Database::NOME,Database::ANIME, "");
    if(hashBuscada.contains(ridAnime))
        return hashBuscada[ridAnime];

    hashBuscada = retornaHash(Database::NOME,Database::MANGA, "");
    if(hashBuscada.contains(ridAnime))
        return hashBuscada[ridAnime];

    hashBuscada = retornaHash(Database::NOME,Database::NOVEL, "");
    if(hashBuscada.contains(ridAnime))
        return hashBuscada[ridAnime];
    return "CURRENT";
}

int DatabaseSearch::buscaIDRetornaPosicao(const QString &ridAnime, Database::type tipo)
{
    int id = -1;
    if(tipo == Database::ANIME)
        id = retornaPosicao(ridAnime,Database::ANIME);
    else if(tipo == Database::MANGA)
        id = retornaPosicao(ridAnime,Database::MANGA);
    else if(tipo == Database::NOVEL)
        id = retornaPosicao(ridAnime,Database::NOVEL);

    return id;
}

int DatabaseSearch::retornaPosicao(const QString &ridAnime, Database::type tipo)
{
    QHash<QString, int> hashPosicao = retornaHash(Database::POSICAO, tipo, 0);
    QString lista = buscaIDRetornaLista(ridAnime, tipo);
    bool addHash = false;

    if(hashPosicao.contains(ridAnime)){
        int lposicao = hashPosicao[ridAnime];
        QVector<anime*> tempList;
        tempList = returnList(lista, tipo);
        //Checa se o a posição do anime na lista está correta. Caso esteja errada, insere na posição certa.
        if(tempList.size() <= lposicao && tempList[lposicao]->vid.compare(ridAnime) != 0)
            addHash = true;
        if(addHash){
            for(int i = 0; i < tempList.size(); i++){
                if(tempList[i]->vid.compare(ridAnime) == 0){
                    //função de inserir no hash
                    insereDataHashPosicao(tipo, ridAnime,i);
                    return i;
                }
            }
        }
        return hashPosicao[ridAnime];
    }
    return -1;
}

QString DatabaseSearch::buscaIDRetornaEpisodio(const QString &rid, Database::type tipo)
{
    QString lista = buscaIDRetornaLista(rid, tipo);
    int lposicao = buscaIDRetornaPosicao(rid, tipo);
    if(lposicao == -1)
        return "0";
    QVector<anime*> tempList;
    tempList = returnList(lista, tipo);
    if(tempList.size() > lposicao)
        return tempList[lposicao]->vnumEpisodiosAssistidos;
    return "0";
}

QString DatabaseSearch::IDRetornaNota(const QString &rid, Database::type tipo)
{
    QString lista = buscaIDRetornaLista(rid, tipo);
    int lposicao = buscaIDRetornaPosicao(rid, tipo);
    if(lposicao == -1)
        return "0";

    QVector<anime*> tempList;
    tempList = returnList(lista, tipo);
    if(tempList.size() > lposicao)
        return tempList[lposicao]->vnotaMediaPessoal;
    return "0";
}

QString DatabaseSearch::IDRetornaTitulo(const QString &rid, Database::type tipo)
{
    QString lista = buscaIDRetornaLista(rid, tipo);
    int lposicao = buscaIDRetornaPosicao(rid, tipo);
    if(lposicao == -1)
        return "0";

    QVector<anime*> tempList;
    tempList = returnList(lista, tipo);
    if(tempList.size() > lposicao)
        return tempList[lposicao]->vnome;
    return "0";
}

QString DatabaseSearch::buscaNomeRetornaID(const QString &rnomeAnime, Database::type tipo)
{
    QHash<QString, QStringList> hashNome = retornaHash(Database::NOME, tipo, QStringList(rnomeAnime));
    foreach(QString key, hashNome.keys()){
        if(hashNome[key].contains(rnomeAnime))
            return key;
    }

    QVector<anime*> tempList;
    foreach(QString lista, nomeListas){
        tempList = returnList(lista, tipo);
        //Checa se a lista não está vazia
        if(!tempList.isEmpty()){
            foreach(anime* anime, tempList){
                if(comparaNomes(anime->vnome, anime->vnomeIngles, anime->vnomeAlternativo, rnomeAnime)){
                    insereNomesHashNomeAnimesPorId(anime->vid, QStringList(rnomeAnime));
                    return anime->vid;
                }
            }
        }
    }
    return "0";
}
