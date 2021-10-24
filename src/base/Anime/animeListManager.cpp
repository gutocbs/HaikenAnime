#include "animelistmanager.h"

AnimeListManager *AnimeListManager::createInstance()
{
    return new AnimeListManager();
}

AnimeListManager::AnimeListManager(QObject *parent) : IMediaListManager(parent)
{

}

AnimeListManager *AnimeListManager::instance()
{
    return Singleton<AnimeListManager>::instance(AnimeListManager::createInstance);
}

IMediaListManager *AnimeListManager::getInstance()
{
    return this->instance();
}

QPointer<Media> AnimeListManager::getMediaById(int id)
{
    return hashMediaById.value(id);
}

QHash<int, Media *> AnimeListManager::getHashMediaById()
{
    return hashMediaById;
}

QVector<Media*> AnimeListManager::getMediaList(Enums::mediaList mediaList, QVariant searchArgument)
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
                return getAnimeYearlyList(searchArgument.toInt());
            }
        }
        break;
    }
    return QVector<Media*>();
}

QVector<Media*> AnimeListManager::getSortList(Enums::mediaList mediaList, QVariant searchArgument)
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
    return tempList;
}

//TODO - Fazer função
QVector<Media *> AnimeListManager::getAnimeYearlyList(int ano)
{
    return QVector<Media*>();
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
    if(MediaComparer::compareName(oficialTitle, searchedTitle) || MediaComparer::compareName(englishTitle, searchedTitle))
        return true;
    foreach(QString alternativeTitle, alternativeTitles){
        if(MediaComparer::compareName(alternativeTitle, searchedTitle))
            return true;
    }
    return false;
}

//TODO - Salvar tudo em um banco de dados ao invés de usar hashs
bool AnimeListManager::addMedia(Media *mediaObject, Enums::mediaList mediaList)
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
    mediaVector->append(mediaObject);
    hashMediaById.insert(mediaObject->id, mediaObject);
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
    hashMediaById.remove(media->id);
    return true;
}

void AnimeListManager::addToHash(QPointer<Media> media)
{
    hashMediaById.insert(media->id, media);
}

Enums::mediaType AnimeListManager::getMediaType()
{
    return Enums::mediaType::ANIME;
}
