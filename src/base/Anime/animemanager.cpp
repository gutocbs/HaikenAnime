#include "animemanager.h"

AnimeManager::AnimeManager(QObject *parent) : QObject(parent)
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

bool AnimeManager::updateMediaList(const QString &mediaId, Enums::mediaList newList)
{
    int index = mediaSearchManager->getMediaListIndexFromId(mediaId);
    if(index == -1)
        return false;
    QString currentList = mediaSearchManager->getMediaListNameFromId(mediaId);

    //Checa se a lista é um número válido
    bool ok;
    currentList.toInt(&ok);

    QPointer<Media> tempMedia(new Media);
    if(!ok){
        Enums::mediaList mediaList = Enums::QStringToMediaList(currentList);
        tempMedia = mediaListManager->getMediaByIndex(mediaList, index);
        if(tempMedia.isNull())
            return false;

        tempMedia->vlista = Enums::enumMediaListToQString(newList);
        mediaListManager->addMedia(tempMedia, newList);
        mediaListManager->addToHash(tempMedia->vid, Enums::enumMediaListToQString(newList));
        mediaListManager->removeMedia(tempMedia, mediaList);
    }
    //oK, no caso, pega listas por ano
    else{
        //TODO - FAZER FUNÇÃO DE LER A LISTA POR ANO
//        tempMedia = fbuscaAnimeNoAno(llista.toInt(), rid);
        if(tempMedia.isNull())
            return false;
        tempMedia->vlista = Enums::enumMediaListToQString(newList);
        mediaListManager->addMedia(tempMedia, newList);
        mediaListManager->addToHash(tempMedia->vid, Enums::enumMediaListToQString(newList));
    }
    return true;
}

bool AnimeManager::updateScore(const QString &mediaId, const QString &newScore)
{

}

bool AnimeManager::updateProgress(const QString &mediaId, const QString &mediaProgress)
{

}

bool AnimeManager::deleteFromList(const QString &mediaId)
{

}

bool AnimeManager::insertCustomName(const QString &mediaId, const QStringList &mediaTitle)
{

}
