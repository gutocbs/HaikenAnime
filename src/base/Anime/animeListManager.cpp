#include "animelistmanager.h"

//AnimeListManager::AnimeListManager(QObject *parent) : QObject(parent)
//{

//}
AnimeListManager *AnimeListManager::createInstance()
{
    return new AnimeListManager();
}

AnimeListManager *AnimeListManager::instance()
{
    return  Singleton<AnimeListManager>::instance(AnimeListManager::createInstance);
}

QHash<QString, int> AnimeListManager::getHash(Enums::hashList, int valorInt)
{
//    if(!vdatabaseReady)
//        return QHash<QString, QStringList>();
    switch(hashList){
    case Enums::LISTA:
        break;
    case Enums::POSICAO:
        return hashMediaIndexById;
    case Enums::NOMEALTERNATIVO:
        break;
    case Enums::NOME:
        break;
    }
    return QHash<QString, int>();
}

QHash<QString, QString> AnimeListManager::getHash(Enums::hashList, QString valorQString)
{
//    if(!vdatabaseReady)
//        return QHash<QString, QStringList>();
    switch(hashList){
    case Enums::LISTA:
        return hashMediaListById;
    case Enums::POSICAO:
        break;
    case Enums::NOMEALTERNATIVO:
        break;
    case Enums::NOME:
        break;
    }
    return QHash<QString, QString>();
}

QHash<QString, QStringList> AnimeListManager::getHash(Enums::hashList hashList, QStringList valorQStringList)
{
//    if(!vdatabaseReady)
//        return QHash<QString, QStringList>();
    switch(hashList){
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
    switch(hashList){
    case Enums::LISTA:
        break;
    case Enums::POSICAO:
            hashMediaIndexById.insert(id, index);
        break;
    case Enums::NOMEALTERNATIVO:
        break;
    case Enums::NOME:
        break;
    }

}

void AnimeListManager::addToHash(QString id, QString mediaList)
{
    switch(hashList){
    case Enums::LISTA:
        hashMediaListById.insert(id, mediaList);
        break;
    case Enums::POSICAO:
        break;
    case Enums::NOMEALTERNATIVO:
        break;
    case Enums::NOME:
        break;
    }
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

QVector<anime *> AnimeListManager::getMediaList(Enums::mediaList mediaList, QString searchArgument)
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
    return QVector<anime*>();
}

QVector<anime *> AnimeListManager::getSortList(Enums::mediaOrder order, Enums::orderType orderType, Enums::mediaList mediaList)
{
    QVector<anime*> tempList = getMediaList(mediaList);
    switch (orderType) {
    case Enums::Ascending:
        switch (order) {
        case Enums::Title:
            std::sort(tempList.begin(),tempList.end(),[](anime* a, anime* b)->bool{ return a->vnome < b->vnome; });
            break;
        case Enums::StartDate:
            std::sort(tempList.begin(),tempList.end(),[](anime* a, anime* b)->bool{ return a->vdataEstreia < b->vdataEstreia;});
            break;
        case Enums::Progress:
            std::sort(tempList.begin(),tempList.end(),[](anime* a, anime* b)->bool{
                return a->vnumEpisodiosAssistidos.toInt() < b->vnumEpisodiosAssistidos.toInt(); });
            break;
        case Enums::Score:
            std::sort(tempList.begin(),tempList.end(),[](anime* a, anime* b)->bool{
                return a->vnotaMediaPessoal.toInt() < b->vnotaMediaPessoal.toInt() ;});
            break;
        case Enums::Type:
            std::sort(tempList.begin(),tempList.end(),[](anime* a, anime* b)->bool{ return a->vformato < b->vformato ;});
            break;
        }
        break;
    case Enums::Descending:
        switch (order) {
        case Enums::Title:
            std::sort(tempList.begin(),tempList.end(),[](anime* a, anime* b)->bool{ return a->vnome > b->vnome; });
            break;
        case Enums::StartDate:
            std::sort(tempList.begin(),tempList.end(),[](anime* a, anime* b)->bool{ return a->vdataEstreia > b->vdataEstreia;});
            break;
        case Enums::Progress:
            std::sort(tempList.begin(),tempList.end(),[](anime* a, anime* b)->bool{
                return a->vnumEpisodiosAssistidos.toInt() > b->vnumEpisodiosAssistidos.toInt(); });
            break;
        case Enums::Score:
            std::sort(tempList.begin(),tempList.end(),[](anime* a, anime* b)->bool{
                return a->vnotaMediaPessoal.toInt() > b->vnotaMediaPessoal.toInt() ;});
            break;
        case Enums::Type:
            std::sort(tempList.begin(),tempList.end(),[](anime* a, anime* b)->bool{ return a->vformato > b->vformato ;});
            break;
        }
        break;
    }
    return tempList;
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



void AnimeListManager::addToHashList(QString mediaId, Enums::mediaList mediaList, Enums::hashList hashList)
{
    QVector<anime*> tempList = getMediaList(mediaList);
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
