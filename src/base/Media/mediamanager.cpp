#include "mediamanager.h"

MediaManager::MediaManager(QObject *parent) : IMediaManager(parent)
{

}

IMediaManager *MediaManager::getInstance()
{

}


void MediaManager::setMediaSearchManager(IMediaSearchManager *mediaSearchManager)
{
    this->mediaSearchManager = mediaSearchManager;
}

void MediaManager::setMediaListManager(IMediaListManager *mediaListManager)
{
    this->mediaListManager = mediaListManager;
}

bool MediaManager::updateMediaList(int mediaId, Enums::mediaList newList)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(mediaId);
    if(tempMedia.isNull())
        return false;

    if(tempMedia->mediaList != Enums::mediaList::YEAR){
        Enums::mediaList mediaList = tempMedia->mediaList;
        tempMedia->mediaList = newList;
        mediaListManager->getInstance()->addMedia(tempMedia, newList);
        mediaListManager->getInstance()->removeMedia(tempMedia, mediaList);
        mediaListManager->getInstance()->addToHash(tempMedia);
    }
    //oK, no caso, pega listas por ano
    else{
        //TODO - FAZER FUNÇÃO DE LER A LISTA POR ANO
//        tempMedia = fbuscaAnimeNoAno(llista.toInt(), rid);
        if(tempMedia.isNull())
            return false;
        tempMedia->mediaList = newList;
        mediaListManager->getInstance()->addMedia(tempMedia, newList);
        mediaListManager->getInstance()->addToHash(tempMedia);
    }
    return true;
}

bool MediaManager::updateScore(int mediaId, const QString &newScore)
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
bool MediaManager::updateProgress(int mediaId, int mediaProgress)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(mediaId);
    if(tempMedia.isNull())
        return false;

    if(tempMedia->mediaList != Enums::mediaList::YEAR){
        tempMedia->progress = mediaProgress;
        if(tempMedia->progress == tempMedia->totalEpisodes)
            updateMediaList(mediaId, Enums::mediaList::COMPLETED);
    }
    return true;
}

bool MediaManager::deleteFromList(int mediaId)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(mediaId);
    if(tempMedia.isNull())
        return false;

    mediaListManager->removeMedia(tempMedia, tempMedia->mediaList);
    return true;
}

bool MediaManager::insertCustomName(int mediaId, const QStringList &mediaTitle)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(mediaId);
    if(tempMedia.isNull())
        return false;

    tempMedia->customNames.append(mediaTitle);
    return true;
}

bool MediaManager::updatePath(int id, const QString &mediaPath)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(id);
    if(tempMedia.isNull())
        return false;

    tempMedia->mediaPath = mediaPath;
    return true;
}
