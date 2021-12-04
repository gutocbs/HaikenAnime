#include "medialistmanager.h"

MediaListManager::MediaListManager(QObject *parent) : IMediaListManager(parent)
{

}

QPointer<Media> MediaListManager::getMediaById(const int &id)
{
    return hashMediaById.value(id);
}

QHash<int, Media *> MediaListManager::getHashMediaById()
{
    return hashMediaById;
}

QVector<Media*> MediaListManager::getMediaList(Enums::mediaList mediaList, QVariant searchArgument)
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
        if(!searchArgument.toString().isEmpty()){
            bool ok;
            //Checa se a lista é um número válido
            searchArgument.toInt(&ok);
            if(ok){
                return getMediaYearlyList(searchArgument.toInt());
            }
        }
        break;
    }
    return QVector<Media*>();
}

bool MediaListManager::sortList(Enums::mediaList mediaList, QVariant searchArgument)
{
    Enums::mediaOrder order = ListOrder::getOrder();
    Enums::orderType orderType = ListOrder::getOrderType();
    QVector<Media*> tempList = getMediaList(mediaList, searchArgument);
    switch (orderType) {
    case Enums::Ascending:
        switch (order) {
        case Enums::Title:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{ return a->originalName < b->originalName; });
            break;
        case Enums::StartDate:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{ return a->startDate < b->startDate;});
            break;
        case Enums::Progress:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{
                return a->progress < b->progress; });
            break;
        case Enums::Score:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{
                return a->personalScore.toInt() < b->personalScore.toInt() ;});
            break;
        case Enums::Type:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{ return a->format < b->format ;});
            break;
        }
        break;
    case Enums::Descending:
        switch (order) {
        case Enums::Title:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{ return a->originalName > b->originalName; });
            break;
        case Enums::StartDate:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{ return a->startDate > b->startDate;});
            break;
        case Enums::Progress:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{
                return a->progress > b->progress; });
            break;
        case Enums::Score:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{
                return a->personalScore.toInt() > b->personalScore.toInt() ;});
            break;
        case Enums::Type:
            std::sort(tempList.begin(),tempList.end(),[](Media* a, Media* b)->bool{ return a->format > b->format ;});
            break;
        }
        break;
    }
    setMediaList(mediaList, tempList);
    return true;
}

bool MediaListManager::setMediaList(Enums::mediaList mediaList, QVector<Media *> media)
{
    switch (mediaList) {
    case Enums::mediaList::CURRENT:
        mediaListCurrent.swap(media);
        break;
    case Enums::mediaList::COMPLETED:
        mediaListCompleted.swap(media);
        break;
    case Enums::mediaList::PAUSED:
        mediaListPaused.swap(media);
        break;
    case Enums::mediaList::DROPPED:
        mediaListDropped.swap(media);
        break;
    case Enums::mediaList::PLANNING:
        mediaListPlanning.swap(media);
        break;
    case Enums::mediaList::SEARCH:
        mediaListSearch.swap(media);
        break;
    default:
        return false;
    }
    return true;
}

int MediaListManager::size(Enums::mediaList mediaList)
{
    switch (mediaList) {
    case Enums::CURRENT:
        return mediaListCurrent.size();
    case Enums::COMPLETED:
        return mediaListCompleted.size();
    case Enums::PAUSED:
        return mediaListPaused.size();
    case Enums::DROPPED:
        return mediaListDropped.size();
    case Enums::PLANNING:
        return mediaListPlanning.size();
    case Enums::SEARCH:
        return mediaListSearch.size();
    case Enums::WINTER:
        return mediaListSeasonWinter.size();
    case Enums::SUMMER:
        return mediaListSeasonSummer.size();
    case Enums::FALL:
        return mediaListSeasonFall.size();
    case Enums::SPRING:
        return mediaListSeasonSpring.size();
    }
    return 0;
}

//TODO - Fazer função
QVector<Media *> MediaListManager::getMediaYearlyList(const int &ano)
{
    return QVector<Media*>();
}

QPointer<Media> MediaListManager::getMediaByIndex(Enums::mediaList mediaList, const int &index)
{
    QVector<Media*> mediaVector = getMediaList(mediaList);
    if(mediaVector.count() > index)
        return mediaVector.at(index);
    return nullptr;
}

bool MediaListManager::compareMedia(const QString &oficialTitle, const QString &englishTitle, QStringList alternativeTitles, const QString &searchedTitle)
{
    if(MediaComparer::compareName(oficialTitle, searchedTitle) || MediaComparer::compareName(englishTitle, searchedTitle))
        return true;
    foreach(QString alternativeTitle, alternativeTitles){
        if(MediaComparer::compareName(alternativeTitle, searchedTitle))
            return true;
    }
    return false;
}

//TODO - Salvar tudo em um banco de dados ao invés de usar hashs
bool MediaListManager::addMedia(Media *mediaObject, Enums::mediaList mediaList)
{
    QVector<Media*>* mediaVector;
    switch (mediaList) {
    case Enums::mediaList::CURRENT:
        mediaVector = &mediaListCurrent;
        break;
    case Enums::mediaList::COMPLETED:
        mediaVector = &mediaListCompleted;
        break;
    case Enums::mediaList::PAUSED:
        mediaVector = &mediaListPaused;
        break;
    case Enums::mediaList::DROPPED:
        mediaVector = &mediaListDropped;
        break;
    case Enums::mediaList::PLANNING:
        mediaVector = &mediaListPlanning;
        break;
    default:
        return false;
    }

    if(hashMediaById.contains(mediaObject->id) && hashMediaById.value(mediaObject->id)->mediaList == mediaList)
        mediaVector->replace(mediaVector->indexOf(hashMediaById.value(mediaObject->id)), mediaObject);
    else if(hashMediaById.contains(mediaObject->id) && hashMediaById.value(mediaObject->id)->mediaList != mediaList){
        removeMedia(hashMediaById.value(mediaObject->id), hashMediaById.value(mediaObject->id)->mediaList);
        mediaVector->append(mediaObject);
    }
    else
        mediaVector->append(mediaObject);
    hashMediaById.insert(mediaObject->id, mediaObject);
    return true;
}

bool MediaListManager::removeMedia(Media* media, Enums::mediaList mediaList)
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
    hashMediaById.remove(media->id);
    return true;
}

bool MediaListManager::containsMedia(const int &id)
{
    return hashMediaById.contains(id);
}

void MediaListManager::setListOrder(QVariant newOrder)
{
    Enums::mediaOrder newOrderEnum = Enums::QStringToMediaOrder(newOrder.toString());
    if(newOrderEnum == ListOrder::getOrder())
        ListOrder::invertOrder();
    else
        ListOrder::setNewOrder(newOrderEnum);
}

void MediaListManager::addToHash(QPointer<Media> media)
{
    hashMediaById.insert(media->id, media);
}

Enums::mediaType MediaListManager::getMediaType()
{
    return Enums::mediaType::ANIME;
}
