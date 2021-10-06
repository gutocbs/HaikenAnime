#include "novelmanager.h"

NovelManager::NovelManager(QObject *parent) : IMediaManager(parent)
{
}

NovelManager *NovelManager::createInstance()
{
    return new NovelManager();
}

NovelManager *NovelManager::instance()
{
    return Singleton<NovelManager>::instance(NovelManager::createInstance);
}

void NovelManager::setMediaSearchManager(IMediaSearchManager *mediaSearchManager)
{
    this->mediaSearchManager = mediaSearchManager;
}

void NovelManager::setMediaListManager(IMediaListManager *mediaListManager)
{
    this->mediaListManager = mediaListManager;
}

bool NovelManager::updateMediaList(int mediaId, Enums::mediaList newList)
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

bool NovelManager::updateScore(int mediaId, const QString &newScore)
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
bool NovelManager::updateProgress(int mediaId, int mediaProgress)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(mediaId);
    if(tempMedia.isNull())
        return false;

    if(tempMedia->mediaList != Enums::mediaList::YEAR){
        tempMedia->progress = mediaProgress;
    }
    return true;
}

bool NovelManager::deleteFromList(int mediaId)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(mediaId);
    if(tempMedia.isNull())
        return false;

    mediaListManager->removeMedia(tempMedia, tempMedia->mediaList);
    return true;
}

bool NovelManager::insertCustomName(int mediaId, const QStringList &mediaTitle)
{
    QPointer<Media> tempMedia = mediaSearchManager->getMediaFromId(mediaId);
    if(tempMedia.isNull())
        return false;

    tempMedia->customNames.append(mediaTitle);
    return true;
}

bool NovelManager::updatePath(int id, const QString &mediaPath)
{

}
