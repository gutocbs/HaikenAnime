#include "animemanager.h"

AnimeManager::AnimeManager(QObject *parent) : IMediaManager(parent)
{
}

void AnimeManager::setMediaSearchManager(IMediaSearchManager *mediaSearchManager)
{
    this->mediaSearchManager = mediaSearchManager;
}

void AnimeManager::setMediaListManager(IMediaListManager *mediaListManager)
{
    this->mediaListManager = mediaListManager;
}

bool AnimeManager::updateMediaList(int mediaId, Enums::mediaList newList)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(mediaId);
    if(tempMedia.isNull())
        return false;

    if(tempMedia->mediaList != Enums::mediaList::YEAR){
        Enums::mediaList mediaList = tempMedia->mediaList;
        tempMedia->mediaList = newList;
        mediaListManager->addMedia(tempMedia, newList);
        mediaListManager->removeMedia(tempMedia, mediaList);
        mediaListManager->addToHash(tempMedia);
    }
    //oK, no caso, pega listas por ano
    else{
        //TODO - FAZER FUNÇÃO DE LER A LISTA POR ANO
//        tempMedia = fbuscaAnimeNoAno(llista.toInt(), rid);
        if(tempMedia.isNull())
            return false;
        tempMedia->mediaList = newList;
        mediaListManager->addMedia(tempMedia, newList);
        mediaListManager->addToHash(tempMedia);
    }
    return true;
}

bool AnimeManager::updateScore(int mediaId, const QString &newScore)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(mediaId);
    if(tempMedia.isNull())
        return false;

    if(tempMedia->mediaList != Enums::mediaList::YEAR){
        tempMedia->personalScore = newScore;
    }
    return true;
}

//TODO - Testar se isso funciona com ponteiros, como está agora
bool AnimeManager::updateProgress(int mediaId, int mediaProgress)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(mediaId);
    if(tempMedia.isNull())
        return false;

    if(tempMedia->mediaList != Enums::mediaList::YEAR){
        tempMedia->progress = mediaProgress;
    }
    return true;
}

bool AnimeManager::deleteFromList(int mediaId)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(mediaId);
    if(tempMedia.isNull())
        return false;

    mediaListManager->removeMedia(tempMedia, tempMedia->mediaList);
    return true;
}

bool AnimeManager::insertCustomName(int mediaId, const QStringList &mediaTitle)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(mediaId);
    if(tempMedia.isNull())
        return false;

    tempMedia->customNames.append(mediaTitle);
    return true;
}
