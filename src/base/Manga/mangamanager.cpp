#include "mangamanager.h"

MangaManager::MangaManager(QObject *parent) : IMediaManager(parent)
{
}

void MangaManager::setMediaSearchManager(IMediaSearchManager *mediaSearchManager)
{
    this->mediaSearchManager = mediaSearchManager;
}

void MangaManager::setMediaListManager(IMediaListManager *mediaListManager)
{
    this->mediaListManager = mediaListManager;
}

bool MangaManager::updateMediaList(int mediaId, Enums::mediaList newList)
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

bool MangaManager::updateScore(int mediaId, const QString &newScore)
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
bool MangaManager::updateProgress(int mediaId, int mediaProgress)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(mediaId);
    if(tempMedia.isNull())
        return false;

    if(tempMedia->mediaList != Enums::mediaList::YEAR){
        tempMedia->progress = mediaProgress;
    }
    return true;
}

bool MangaManager::deleteFromList(int mediaId)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(mediaId);
    if(tempMedia.isNull())
        return false;

    mediaListManager->removeMedia(tempMedia, tempMedia->mediaList);
    return true;
}

bool MangaManager::insertCustomName(int mediaId, const QStringList &mediaTitle)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(mediaId);
    if(tempMedia.isNull())
        return false;

    tempMedia->customNames.append(mediaTitle);
    return true;
}
