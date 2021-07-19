#include "animemanager.h"

AnimeManager::AnimeManager(QObject *parent) : QObject(parent)
{
    animeSearchManager = new AnimeSearchManager();
}

//Pega o anime e salva em um novo objeto
//Salva o anime na lista nova
//Remove o anime da lista antiga
bool AnimeManager::updateMediaList(const QString &mediaId, Enums::mediaList newList)
{
    int index = animeSearchManager->getMediaListIndexFromId(mediaId);
    if(index == -1)
        return false;
    QString currentList = animeSearchManager->getMediaListNameFromId(mediaId);

    //Checa se a lista é um número válido
    bool ok;
    currentList.toInt(&ok);

    QPointer<Media> tempMedia(new Media);
    if(!ok){
        Enums::mediaList mediaList = Enums::QStringToMediaList(currentList);
        tempMedia = animeListManager->instance()->getMediaByIndex(mediaList, index);
        if(tempMedia.isNull())
            return false;

        tempMedia->vlista = Enums::enumMediaListToQString(newList);
        animeListManager->instance()->addMedia(tempMedia, newList);
        animeListManager->instance()->addToHash(tempMedia->vid, Enums::enumMediaListToQString(newList));
        animeListManager->instance()->removeMedia(tempMedia, mediaList);
    }
    //oK, no caso, pega listas por ano
    else{
        //TODO - FAZER FUNÇÃO DE LER A LISTA POR ANO
//        tempMedia = fbuscaAnimeNoAno(llista.toInt(), rid);
        if(tempMedia.isNull())
            return false;
        tempMedia->vlista = Enums::enumMediaListToQString(newList);
        animeListManager->instance()->addMedia(tempMedia, newList);
        animeListManager->instance()->addToHash(tempMedia->vid, Enums::enumMediaListToQString(newList));
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
