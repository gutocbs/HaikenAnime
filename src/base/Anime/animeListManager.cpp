#include "animelistmanager.h"

AnimeListManager::AnimeListManager(QObject *parent) : IMediaListManager(parent)
{

}

QHash<QString, int> AnimeListManager::getHashMediaIndexById()
{
//    if(!vdatabaseReady)
//        return QHash<QString, QStringList>();
    return hashMediaIndexById;
}

QHash<QString, QString> AnimeListManager::getHashMediaListById()
{
//    if(!vdatabaseReady)
//        return QHash<QString, QStringList>();
    return hashMediaListById;
}

QHash<QString, QStringList> AnimeListManager::getHashMediaNamesById(Enums::hashList mediaList)
{
//    if(!vdatabaseReady)
//        return QHash<QString, QStringList>();
    switch(mediaList){
    case Enums::LISTA:
        break;
    case Enums::POSICAO:
        break;
    case Enums::NOMEALTERNATIVO:
        return hashMediaCustomNamesById;
    case Enums::NOME:
        return hashMediaNameById;
    }
    return QHash<QString, QStringList>();
}

void AnimeListManager::addToHash(QString id, int index)
{
    hashMediaIndexById.insert(id, index);
}

void AnimeListManager::addToHash(QString id, QString mediaList)
{
    hashMediaListById.insert(id, mediaList);
}

void AnimeListManager::addToHash(QString id, QStringList mediaNames, Enums::hashList hashList)
{
    switch(hashList){
    case Enums::LISTA:
        break;
    case Enums::POSICAO:
        break;
    case Enums::NOMEALTERNATIVO:
        hashMediaCustomNamesById.insert(id, mediaNames);
        break;
    case Enums::NOME:
        if(!hashMediaNameById.contains(id))
            hashMediaNameById.insert(id, mediaNames);
        else
            hashMediaNameById[id].append(mediaNames);
        break;
    }
}

QVector<Media*> AnimeListManager::getMediaList(Enums::mediaList mediaList, QString searchArgument)
{
    switch (mediaList) {
    case Enums::CURRENT:
        return mediaListCurrent;
    case Enums::COMPLETED:
        return mediaListCompleted;
    case Enums::PAUSED:
        return mediaListPaused;
    case Enums::DROPPED:
        return mediaListDropped;
    case Enums::PLANNING:
        return mediaListPlanning;
    case Enums::SEARCH:
        return mediaListSearch;
    case Enums::WINTER:
        return mediaListSeasonWinter;
    case Enums::SUMMER:
        return mediaListSeasonSummer;
    case Enums::FALL:
        return mediaListSeasonFall;
    case Enums::SPRING:
        return mediaListSeasonSpring;
    case Enums::YEAR:
        if(!searchArgument.isEmpty()){
            bool ok;
            //Checa se a lista é um número válido
            searchArgument.toInt(&ok);
            if(ok){
    //            return returnAnimeYearlyList(listString.toInt());
            }
        }
        break;
    }
    return QVector<Media*>();
}

QVector<Media*> AnimeListManager::getSortList(Enums::mediaOrder order, Enums::orderType orderType, Enums::mediaList mediaList)
{
    QVector<Media*> tempList = getMediaList(mediaList);
    switch (orderType) {
    case Enums::Ascending:
        switch (order) {
        case Enums::Title:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{ return a->vnome < b->vnome; });
            break;
        case Enums::StartDate:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{ return a->vdataEstreia < b->vdataEstreia;});
            break;
        case Enums::Progress:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{
                return a->vnumEpisodiosAssistidos.toInt() < b->vnumEpisodiosAssistidos.toInt(); });
            break;
        case Enums::Score:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{
                return a->vnotaMediaPessoal.toInt() < b->vnotaMediaPessoal.toInt() ;});
            break;
        case Enums::Type:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{ return a->vformato < b->vformato ;});
            break;
        }
        break;
    case Enums::Descending:
        switch (order) {
        case Enums::Title:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{ return a->vnome > b->vnome; });
            break;
        case Enums::StartDate:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{ return a->vdataEstreia > b->vdataEstreia;});
            break;
        case Enums::Progress:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{
                return a->vnumEpisodiosAssistidos.toInt() > b->vnumEpisodiosAssistidos.toInt(); });
            break;
        case Enums::Score:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{
                return a->vnotaMediaPessoal.toInt() > b->vnotaMediaPessoal.toInt() ;});
            break;
        case Enums::Type:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{ return a->vformato > b->vformato ;});
            break;
        }
        break;
    }
    return tempList;
}

QPointer<Media> AnimeListManager::getMediaByIndex(Enums::mediaList mediaList, int index)
{
    QVector<Media*> mediaVector = getMediaList(mediaList);
    if(mediaVector.count() > index)
        return mediaVector.at(index);
    return nullptr;
}

bool AnimeListManager::compareMedia(QString oficialTitle, QString englishTitle, QStringList alternativeTitles, QString searchedTitle)
{
    if(FormataPalavras::fcomparaNomes(oficialTitle, searchedTitle) || FormataPalavras::fcomparaNomes(englishTitle, searchedTitle))
        return true;
    foreach(QString alternativeTitle, alternativeTitles){
        if(FormataPalavras::fcomparaNomes(alternativeTitle, searchedTitle))
            return true;
    }
    return false;
}

bool AnimeListManager::addMedia(Media *mediaObject, Enums::mediaList mediaList)
{
    switch (mediaList) {
    case Enums::mediaList::CURRENT:
        mediaListCurrent.append(mediaObject);
        hashMediaIndexById.insert(mediaObject->vid, mediaListCurrent.indexOf(mediaListCurrent.last()));
        break;
    case Enums::mediaList::COMPLETED:
        mediaListCompleted.append(mediaObject);
        hashMediaIndexById.insert(mediaObject->vid, mediaListCompleted.indexOf(mediaListCompleted.last()));
        break;
    case Enums::mediaList::PAUSED:
        mediaListPaused.append(mediaObject);
        hashMediaIndexById.insert(mediaObject->vid, mediaListPaused.indexOf(mediaListPaused.last()));
        break;
    case Enums::mediaList::DROPPED:
        mediaListDropped.append(mediaObject);
        hashMediaIndexById.insert(mediaObject->vid, mediaListDropped.indexOf(mediaListDropped.last()));
        break;
    case Enums::mediaList::PLANNING:
        mediaListPlanning.append(mediaObject);
        hashMediaIndexById.insert(mediaObject->vid, mediaListPlanning.indexOf(mediaListPlanning.last()));
        break;
    default:
        return false;
    }
    return true;
}

bool AnimeListManager::removeMedia(Media* media, Enums::mediaList mediaList)
{
    switch (mediaList) {
    case Enums::mediaList::CURRENT:
        mediaListCurrent.removeOne(media);
        break;
    case Enums::mediaList::COMPLETED:
        mediaListCompleted.removeOne(media);
        break;
    case Enums::mediaList::PAUSED:
        mediaListPaused.removeOne(media);
        break;
    case Enums::mediaList::DROPPED:
        mediaListDropped.removeOne(media);
        break;
    case Enums::mediaList::PLANNING:
        mediaListPlanning.removeOne(media);
        break;
    default:
        return false;
    }
    return true;
}

void AnimeListManager::addToHashList(QString mediaId, Enums::mediaList mediaList, Enums::hashList hashList)
{
    QVector<Media*> tempList = getMediaList(mediaList);
    for(int i = 0; i < tempList.size(); i++){
        if(tempList[i]->vid.compare(mediaId) == 0){
            switch(hashList){
            case Enums::LISTA:
                addToHash(mediaId, Enums::enumMediaListToQString(mediaList));
                break;
            case Enums::POSICAO:
                addToHash(mediaId, i);
                break;
            case Enums::NOMEALTERNATIVO:
                addToHash(mediaId, tempList[i]->vnomeAlternativo, hashList);
                break;
            case Enums::NOME:
                addToHash(mediaId, QStringList(tempList[i]->vnome), hashList);
                break;
            }
        }
    }
}
