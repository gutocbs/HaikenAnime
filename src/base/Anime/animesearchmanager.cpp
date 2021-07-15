#include "animesearchmanager.h"

//int AnimeSearchManager::buscaIDRetornaPosicao(const QString &idAnime)
//{
//    int id = -1;
////Passar AnimeSearchManager pelo construtor. Fazer uma classe que irá setar todos as classes por tipos

//    return id;
//}

void AnimeSearchManager::loadListNames()
{
    listNames.append("CURRENT");
    listNames.append("COMPLETED");
    listNames.append("PLANNING");
    listNames.append("DROPPED");
    listNames.append("PAUSED");
}

QString AnimeSearchManager::buscaIDRapido(const QString &rnomeAnime)
{
    QHash<QString, QString> hashBuscada = animeManager.getHash(hashList, "");
    foreach(QString key, hashBuscada.keys()){
        if(hashBuscada[key].contains(rnomeAnime))
            return key;
    }
    return "";
}

QString AnimeSearchManager::getListFromId(const QString &idAnime)
{
    QHash<QString, QString> hashBuscada = animeManager.getHash(hashList, "");
    if(hashBuscada.contains(idAnime))
        return hashBuscada[idAnime];
    return "CURRENT";
}

int AnimeSearchManager::getListIndexFromId(const QString &idAnime)
{
    QHash<QString, int> hashIndex = animeManager.getHash(hashList, 0);
    QString list = getListFromId(idAnime);
    bool addHash = false;

    if(hashIndex.contains(idAnime)){
        QPointer<anime> animeMedia = getMediaFromId(idAnime);
        //Checa se o a posição do anime na lista está correta. Caso esteja errada, insere na posição certa.
        if(animeMedia->vid.compare(idAnime) != 0){
            Enums::mediaList listEnum = getListEnum(list);
            animeManager.addToHashList(idAnime, listEnum, Enums::POSICAO);
            hashIndex = animeManager.getHash(hashList, 0);
        }
        return hashIndex[idAnime];
    }
    return -1;
}

QPointer<anime> AnimeSearchManager::getMediaFromId(const QString &idAnime){
    QString lista = getListFromId(idAnime);
    int listIndex = getListIndexFromId(idAnime);
    if(listIndex == -1)
        return nullptr;

    QVector<anime*> tempList;
    tempList = animeManager.getList(lista);
    if(tempList.size() > listIndex)
        return tempList[listIndex];

    return nullptr;
}

QVector<anime*> AnimeSearchManager::getMediaListFromId(const QString &idAnime){
    QString lista = getListFromId(idAnime);
    int listIndex = getListIndexFromId(idAnime);
    if(listIndex == -1)
        return QVector<anime*>();

    QVector<anime*> tempList;
    tempList = animeManager.getList(lista);
    if(tempList.size() > listIndex)
        return tempList;

    return QVector<anime*>();
}

QString AnimeSearchManager::getMediaEpisodeFromId(const QString &idAnime)
{
    QPointer<anime> anime = getMediaFromId(idAnime);
    if(!anime.isNull())
        return anime->vnumEpisodiosAssistidos;
    return "0";
}

QString AnimeSearchManager::getMediaScoreFromId(const QString &idAnime)
{
    QPointer<anime> anime = getMediaFromId(idAnime);
    if(!anime.isNull())
        return anime->vnotaMediaPessoal;
    return "0";
}

QString AnimeSearchManager::getMediaTitleFromId(const QString &idAnime)
{
    QPointer<anime> anime = getMediaFromId(idAnime);
    if(!anime.isNull())
        return anime->vnome;
    return "0";
}

QString AnimeSearchManager::getIdFromMediaTitle(const QString &mediaTitle)
{
    QHash<QString, QStringList> hashNome = animeManager.getHash(hashList, QStringList(mediaTitle));
    foreach(QString key, hashNome.keys()){
        if(hashNome[key].contains(mediaTitle))
            return key;
    }

    QVector<anime*> tempList;
    foreach(QString list, listNames){
        tempList = animeManager.getList(list);
        //Checa se a lista não está vazia
        if(!tempList.isEmpty()){
            foreach(anime* anime, tempList){
                if(animeManager.compareMedia(anime->vnome, anime->vnomeIngles, anime->vnomeAlternativo, mediaTitle)){
                    insereNomesHashNomeAnimesPoidAnime(anime->vid, QStringList(mediaTitle));
                    animeManager.addToHashList(anime->vid, listEnum, Enums::POSICAO);
                    return anime->vid;
                }
            }
        }
    }
    return "0";
}
