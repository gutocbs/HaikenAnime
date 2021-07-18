#include "animemanager.h"

//AnimeManager::AnimeManager(QObject *parent) : QObject(parent)
//{

//}
QHash<QString, int> AnimeManager::getHash(Enums::hashList, int valorInt)
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

QHash<QString, QString> AnimeManager::getHash(Enums::hashList, QString valorQString)
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

QHash<QString, QStringList> AnimeManager::getHash(Enums::hashList hashList, QStringList valorQStringList)
{
//    if(!vdatabaseReady)
//        return QHash<QString, QStringList>();
    switch(hashList){
    case Enums::LISTA:
        break;
    case Enums::POSICAO:
        break;
    case Enums::NOMEALTERNATIVO:
        return hashMediaCustomNames;
    case Enums::NOME:
        return hashMediaNameById;
    }
    return QHash<QString, QStringList>();
}

QVector<anime *> AnimeManager::getMediaList(Enums::mediaList mediaList, QString searchArgument)
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

QVector<anime *> AnimeManager::getSortList(Enums::mediaOrder order, Enums::orderType orderType, Enums::mediaList mediaList)
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



void AnimeManager::addToHashList(QString mediaId, Enums::mediaList mediaList, Enums::hashList hashList)
{
    QVector<anime*> tempList = getMediaList(mediaList);
    for(int i = 0; i < tempList.size(); i++){
        if(tempList[i]->vid.compare(mediaId) == 0){
            //função de inserir no hash
            //TODO - Criar função de inserir dados nos hashs caso não existam lá
//            return i;
        }
    }
}
