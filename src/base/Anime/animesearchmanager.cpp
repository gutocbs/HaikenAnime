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

QVector<anime *> AnimeSearchManager::getMediaList(const QString &listName){
    Enums::mediaList listEnum = Enums::QStringToMediaList(listName);
    return animeManager.getMediaList(listEnum);
}

QVector<anime *> AnimeSearchManager::searchMedia(const QString &rnome)
{
    QVector<anime*> mediaListSearch = animeManager.getMediaList(Enums::SEARCH);
    mediaListSearch.clear();
//    if(!vdatabaseReady)
//        return mediaListSearch;
    QHash<QString, QStringList> hashMediaNameById = animeManager.getHash(Enums::NOME, QStringList());
    QHash<QString, QString> hashMediaListById = animeManager.getHash(Enums::LISTA, "");
    QHash<QString, int> hashMediaIndexById = animeManager.getHash(Enums::POSICAO, 0);
    QVector<anime*> tempList;
    foreach(QString key, hashMediaNameById.keys()){
//        if(!vdatabaseReady)
//            return mediaListSearch;
        for(int i = 0; i < hashMediaNameById[key].size(); i++){
            if(hashMediaNameById[key].at(i).contains(rnome, Qt::CaseInsensitive)){
                QString list = hashMediaListById[key];
                int pos = hashMediaIndexById[key];
                if(pos == -1)
                    break;
                else if(list.compare(Enums::enumMediaListToQString(Enums::CURRENT), Qt::CaseInsensitive) == 0){
                    appendToList(mediaListSearch, Enums::CURRENT, pos);
                    break;
                }
                else if(list.compare(Enums::enumMediaListToQString(Enums::PLANNING), Qt::CaseInsensitive) == 0){
                    appendToList(mediaListSearch, Enums::PLANNING, pos);
                    break;
                }
                else if(list.compare(Enums::enumMediaListToQString(Enums::PAUSED), Qt::CaseInsensitive) == 0){
                    appendToList(mediaListSearch, Enums::PAUSED, pos);
                    break;
                }
                else if(list.compare(Enums::enumMediaListToQString(Enums::DROPPED), Qt::CaseInsensitive) == 0){
                    appendToList(mediaListSearch, Enums::DROPPED, pos);
                    break;
                }
                else if(list.compare(Enums::enumMediaListToQString(Enums::COMPLETED), Qt::CaseInsensitive) == 0){
                    appendToList(mediaListSearch, Enums::COMPLETED, pos);
                    break;
                }
                else{
                    bool ok;
                    //Checa se a list é um número válido
                    list.toInt(&ok);
                    if(ok){
                        mediaListSearch.append(fbuscaAnimeNoAno(list.toInt(), key));
                    }
                    break;
                }
            }
        }
    }
    return mediaListSearch;
}

void AnimeSearchManager::appendToList(QVector<anime*> &mediaList, Enums::mediaList list, int position){
    QVector<anime*> tempList = animeManager.getMediaList(list);
    if(tempList.size() > position)
        mediaList.append(tempList[position]);
}

QString AnimeSearchManager::buscaIDRapido(const QString &rnomeAnime)
{
    //TODO - Especificar hash buscada
    QHash<QString, QString> hashBuscada = animeManager.getHash(hashList, "");
    foreach(QString key, hashBuscada.keys()){
        if(hashBuscada[key].contains(rnomeAnime))
            return key;
    }
    return "";
}

QString AnimeSearchManager::getMediaListNameFromId(const QString &idAnime)
{
    QHash<QString, QString> hashBuscada = animeManager.getHash(hashList, "");
    if(hashBuscada.contains(idAnime))
        return hashBuscada[idAnime];
    return "CURRENT";
}

int AnimeSearchManager::getMediaListIndexFromId(const QString &idAnime)
{
    QHash<QString, int> hashIndex = animeManager.getHash(hashList, 0);
    QString list = getMediaListNameFromId(idAnime);

    if(hashIndex.contains(idAnime)){
        QPointer<anime> animeMedia = getMediaFromId(idAnime);
        //Checa se o a posição do anime na lista está correta. Caso esteja errada, insere na posição certa.
        if(animeMedia->vid.compare(idAnime) != 0){
            Enums::mediaList listEnum = Enums::QStringToMediaList(list);
            animeManager.addToHashList(idAnime, listEnum, Enums::POSICAO);
            hashIndex = animeManager.getHash(hashList, 0);
        }
        return hashIndex[idAnime];
    }
    return -1;
}

QPointer<anime> AnimeSearchManager::getMediaFromId(const QString &idAnime){
    QString lista = getMediaListNameFromId(idAnime);
    int listIndex = getMediaListIndexFromId(idAnime);
    if(listIndex == -1)
        return nullptr;

    QVector<anime*> tempList = getMediaList(lista);
    if(tempList.size() > listIndex)
        return tempList[listIndex];

    return nullptr;
}

QVector<anime*> AnimeSearchManager::getMediaListFromId(const QString &idAnime){
    QString lista = getMediaListNameFromId(idAnime);
    int listIndex = getMediaListIndexFromId(idAnime);
    if(listIndex == -1)
        return QVector<anime*>();

    QVector<anime*> tempList = getMediaList(lista);
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
        tempList = getMediaList(list);
        //Checa se a lista não está vazia
        if(!tempList.isEmpty()){
            for(int i = 0; i < tempList.size(); i++){
                if(animeManager.compareMedia(tempList[i]->vnome, tempList[i]->vnomeIngles, tempList[i]->vnomeAlternativo, mediaTitle)){
                    animeManager.addToHash(tempList[i]->vid, QStringList(tempList[i]->vnomeAlternativo), Enums::NOMEALTERNATIVO);
                    animeManager.addToHash(tempList[i]->vid, QStringList(mediaTitle), Enums::NOMEALTERNATIVO);
                    animeManager.addToHash(tempList[i]->vid, list);
                    animeManager.addToHash(tempList[i]->vid, i);
                    return tempList[i]->vid;
                }
            }
        }
    }
    return "0";
}
