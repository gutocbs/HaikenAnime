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
    QString list = mediaSearchManager->getMediaListNameFromId(mediaId);
    int index = mediaSearchManager->getMediaListIndexFromId(mediaId);
    if(index == -1)
        return false;
    bool ok;
    //Checa se a lista é um número válido
    list.toInt(&ok);
    Enums::mediaList mediaList = Enums::CURRENT;
    if(list.compare(Enums::enumMediaListToQString(Enums::CURRENT), Qt::CaseInsensitive) == 0)
        mediaList = Enums::CURRENT;
    else if(list.compare(Enums::enumMediaListToQString(Enums::COMPLETED), Qt::CaseInsensitive) == 0)
        mediaList = Enums::COMPLETED;
    else if(list.compare(Enums::enumMediaListToQString(Enums::PAUSED), Qt::CaseInsensitive) == 0)
        mediaList = Enums::PAUSED;
    else if(list.compare(Enums::enumMediaListToQString(Enums::DROPPED), Qt::CaseInsensitive) == 0)
        mediaList = Enums::DROPPED;
    else if(list.compare(Enums::enumMediaListToQString(Enums::PLANNING), Qt::CaseInsensitive) == 0)
        mediaList = Enums::PLANNING;
    else if(ok){
        QPointer<anime> lnovoAnime(new anime);
        //TODO - Implementar função de busca por ano
//        lnovoAnime = fbuscaAnimeNoAno(list.toInt(), mediaId);
//        lnovoAnime->vnotaMediaPessoal = newScore;
//        return true;
    }
    QVector<Media*> mediaListVector = mediaListManager->getMediaList(mediaList);
    mediaListVector[index]->vnotaMediaPessoal = newScore;
    return true;
}

bool AnimeManager::updateProgress(const QString &mediaId, const QString &mediaProgress)
{
    QString list = mediaSearchManager->getMediaListNameFromId(mediaId);
    int index = mediaSearchManager->getMediaListIndexFromId(mediaId);
    if(index == -1)
        return false;
    bool ok;
    //Checa se a lista é um número válido
    list.toInt(&ok);
    Enums::mediaList mediaList = Enums::CURRENT;
    if(list.compare(Enums::enumMediaListToQString(Enums::CURRENT), Qt::CaseInsensitive) == 0)
        mediaList = Enums::CURRENT;
    else if(list.compare(Enums::enumMediaListToQString(Enums::COMPLETED), Qt::CaseInsensitive) == 0)
        mediaList = Enums::COMPLETED;
    else if(list.compare(Enums::enumMediaListToQString(Enums::PAUSED), Qt::CaseInsensitive) == 0)
        mediaList = Enums::PAUSED;
    else if(list.compare(Enums::enumMediaListToQString(Enums::DROPPED), Qt::CaseInsensitive) == 0)
        mediaList = Enums::DROPPED;
    else if(list.compare(Enums::enumMediaListToQString(Enums::PLANNING), Qt::CaseInsensitive) == 0)
        mediaList = Enums::PLANNING;
    else if(ok){
        QPointer<anime> lnovoAnime(new anime);
        //TODO - Implementar função de busca por ano
//        lnovoAnime = fbuscaAnimeNoAno(list.toInt(), mediaId);
//        lnovoAnime->vnotaMediaPessoal = newScore;
//        return true;
    }
    QVector<Media*> mediaListVector = mediaListManager->getMediaList(mediaList);
    mediaListVector[index]->vnumEpisodiosAssistidos = mediaProgress;
    return true;
}

bool AnimeManager::deleteFromList(const QString &mediaId)
{
    QString list = mediaSearchManager->getMediaListNameFromId(mediaId);
    int index = mediaSearchManager->getMediaListIndexFromId(mediaId);
    if(index == -1)
        return false;
    Enums::mediaList mediaList = Enums::CURRENT;
    if(list.compare(Enums::enumMediaListToQString(Enums::CURRENT), Qt::CaseInsensitive) == 0)
        mediaList = Enums::CURRENT;
    else if(list.compare(Enums::enumMediaListToQString(Enums::COMPLETED), Qt::CaseInsensitive) == 0)
        mediaList = Enums::COMPLETED;
    else if(list.compare(Enums::enumMediaListToQString(Enums::PAUSED), Qt::CaseInsensitive) == 0)
        mediaList = Enums::PAUSED;
    else if(list.compare(Enums::enumMediaListToQString(Enums::DROPPED), Qt::CaseInsensitive) == 0)
        mediaList = Enums::DROPPED;
    else if(list.compare(Enums::enumMediaListToQString(Enums::PLANNING), Qt::CaseInsensitive) == 0)
        mediaList = Enums::PLANNING;
    QVector<Media*> mediaListVector = mediaListManager->getMediaList(mediaList);
    mediaListManager->removeMedia(mediaListVector[index], mediaList);
    return true;
}

bool AnimeManager::insertCustomName(const QString &mediaId, const QStringList &mediaTitle)
{
    QString list = mediaSearchManager->getMediaListNameFromId(mediaId);
    int index = mediaSearchManager->getMediaListIndexFromId(mediaId);
    if(index == -1)
        return false;
    bool ok;
    //Checa se a lista é um número válido
    list.toInt(&ok);
    Enums::mediaList mediaList = Enums::CURRENT;
    if(list.compare(Enums::enumMediaListToQString(Enums::CURRENT), Qt::CaseInsensitive) == 0)
        mediaList = Enums::CURRENT;
    else if(list.compare(Enums::enumMediaListToQString(Enums::COMPLETED), Qt::CaseInsensitive) == 0)
        mediaList = Enums::COMPLETED;
    else if(list.compare(Enums::enumMediaListToQString(Enums::PAUSED), Qt::CaseInsensitive) == 0)
        mediaList = Enums::PAUSED;
    else if(list.compare(Enums::enumMediaListToQString(Enums::DROPPED), Qt::CaseInsensitive) == 0)
        mediaList = Enums::DROPPED;
    else if(list.compare(Enums::enumMediaListToQString(Enums::PLANNING), Qt::CaseInsensitive) == 0)
        mediaList = Enums::PLANNING;
    else if(ok){
        QPointer<anime> lnovoAnime(new anime);
        //TODO - Implementar função de busca por ano
//        lnovoAnime = fbuscaAnimeNoAno(list.toInt(), mediaId);
//        lnovoAnime->vnotaMediaPessoal = newScore;
//        return true;
    }
    QVector<Media*> mediaListVector = mediaListManager->getMediaList(mediaList);
    mediaListVector[index]->vnomeAlternativo.append(mediaTitle);
    return true;
}
