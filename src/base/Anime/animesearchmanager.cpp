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

QVector<Media*> AnimeSearchManager::getMediaList(const QString &listName){
    Enums::mediaList listEnum = Enums::QStringToMediaList(listName);
    return animeManager.getMediaList(listEnum);
}

QVector<Media*> AnimeSearchManager::searchMedia(const QString &rnome)
{
    QVector<Media*> mediaListSearch = animeManager.getMediaList(Enums::SEARCH);
    mediaListSearch.clear();
//    if(!vdatabaseReady)
//        return mediaListSearch;
    QHash<QString, QStringList> hashMediaNameById = animeManager.getHashMediaNamesById(Enums::NOME);
    QHash<QString, QString> hashMediaListById = animeManager.getHashMediaListById();
    QHash<QString, int> hashMediaIndexById = animeManager.getHashMediaIndexById();
    QHash<QString, QStringList>::iterator iterator;
    for (iterator = hashMediaNameById.begin(); iterator != hashMediaNameById.end(); ++iterator){
//        if(!vdatabaseReady)
//            return mediaListSearch;
        for(int i = 0; i < hashMediaNameById[iterator.key()].size(); i++){
            if(hashMediaNameById[iterator.key()].at(i).contains(rnome, Qt::CaseInsensitive)){
                QString list = hashMediaListById[iterator.key()];
                int pos = hashMediaIndexById[iterator.key()];
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
                        mediaListSearch.append(fbuscaAnimeNoAno(list.toInt(), iterator.key()));
                    }
                    break;
                }
            }
        }
    }
    return mediaListSearch;
}

void AnimeSearchManager::appendToList(QVector<Media*> &mediaList, Enums::mediaList list, int position){
    QVector<Media*> tempList = animeManager.getMediaList(list);
    if(tempList.size() > position)
        mediaList.append(tempList[position]);
}

QString AnimeSearchManager::buscaIDRapido(const QString &rnomeAnime)
{
    QHash<QString, QString> hashBuscada = animeManager.getHashMediaListById();

    QHash<QString, QString>::iterator iterator;
    for (iterator = hashBuscada.begin(); iterator != hashBuscada.end(); ++iterator){
        if(hashBuscada[iterator.key()].contains(rnomeAnime))
            return iterator.key();
    }
    return "";
}

QString AnimeSearchManager::getMediaListNameFromId(const QString &idAnime)
{
    QHash<QString, QString> hashBuscada = animeManager.getHashMediaListById();
    if(hashBuscada.contains(idAnime))
        return hashBuscada[idAnime];
    return "CURRENT";
}

int AnimeSearchManager::getMediaListIndexFromId(const QString &idAnime)
{
    QHash<QString, int> hashIndex = animeManager.getHashMediaIndexById();
    QString list = getMediaListNameFromId(idAnime);

    if(hashIndex.contains(idAnime)){
        QPointer<Media> animeMedia = getMediaFromId(idAnime);
        //Checa se o a posição do anime na lista está correta. Caso esteja errada, insere na posição certa.
        if(animeMedia->vid.compare(idAnime) != 0){
            Enums::mediaList listEnum = Enums::QStringToMediaList(list);
            animeManager.addToHashList(idAnime, listEnum, Enums::hashList::POSICAO);
            hashIndex = animeManager.getHashMediaIndexById();
        }
        return hashIndex[idAnime];
    }
    return -1;
}

QPointer<Media> AnimeSearchManager::getMediaFromId(const QString &idAnime){
    QString lista = getMediaListNameFromId(idAnime);
    int listIndex = getMediaListIndexFromId(idAnime);
    if(listIndex == -1)
        return nullptr;

    QVector<Media*> tempList = getMediaList(lista);
    if(tempList.size() > listIndex)
        return tempList[listIndex];

    return nullptr;
}

QVector<Media*> AnimeSearchManager::getMediaListFromId(const QString &idAnime){
    QString lista = getMediaListNameFromId(idAnime);
    int listIndex = getMediaListIndexFromId(idAnime);
    if(listIndex == -1)
        return QVector<Media*>();

    QVector<Media*> tempList = getMediaList(lista);
    if(tempList.size() > listIndex)
        return tempList;

    return QVector<Media*>();
}

QString AnimeSearchManager::getMediaEpisodeFromId(const QString &idAnime)
{
    QPointer<Media> anime = getMediaFromId(idAnime);
    if(!anime.isNull())
        return anime->vnumEpisodiosAssistidos;
    return "0";
}

QString AnimeSearchManager::getMediaScoreFromId(const QString &idAnime)
{
    QPointer<Media> anime = getMediaFromId(idAnime);
    if(!anime.isNull())
        return anime->vnotaMediaPessoal;
    return "0";
}

QString AnimeSearchManager::getMediaTitleFromId(const QString &idAnime)
{
    QPointer<Media> anime = getMediaFromId(idAnime);
    if(!anime.isNull())
        return anime->vnome;
    return "0";
}

QString AnimeSearchManager::getIdFromMediaTitle(const QString &mediaTitle)
{
    QHash<QString, QStringList> hashNome = animeManager.getHashMediaNamesById(Enums::hashList::NOME);
    QHash<QString, QStringList>::iterator iterator;
    for (iterator = hashNome.begin(); iterator != hashNome.end(); ++iterator){
        if(hashNome[iterator.key()].contains(mediaTitle))
            return iterator.key();
    }

    QVector<Media*> tempList;
    foreach(QString list, listNames){
        tempList = getMediaList(list);
        //Checa se a lista não está vazia
        if(!tempList.isEmpty()){
            for(int i = 0; i < tempList.size(); i++){
                if(animeManager.compareMedia(tempList[i]->vnome, tempList[i]->vnomeIngles, tempList[i]->vnomeAlternativo, mediaTitle)){
                    animeManager.addToHash(tempList[i]->vid, QStringList(tempList[i]->vnomeAlternativo), Enums::hashList::NOMEALTERNATIVO);
                    animeManager.addToHash(tempList[i]->vid, QStringList(mediaTitle), Enums::hashList::NOMEALTERNATIVO);
                    animeManager.addToHash(tempList[i]->vid, list);
                    animeManager.addToHash(tempList[i]->vid, i);
                    return tempList[i]->vid;
                }
            }
        }
    }
    return "0";
}
