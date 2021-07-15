#include "animemanager.h"

//AnimeManager::AnimeManager(QObject *parent) : QObject(parent)
//{

//}

QHash<QString, QStringList> AnimeManager::getHash(Enums::hashList hashList)
{
//    if(!vdatabaseReady)
//        return QHash<QString, QStringList>();
    switch(hashList){
    case Enums::LISTA:
        break;
    case Enums::POSICAO:
        break;
    case Enums::NOMEALTERNATIVO:
        break;
    case Enums::NOME:
//        return vhashNomeAnimesPorId;
        break;
    }
    return QHash<QString, QStringList>();
}

void AnimeManager::addToHashList(QString mediaId, Enums::mediaList mediaList, Enums::hashList hashList)
{
    QVector<anime*> tempList = getMediaList(mediaList);
    for(int i = 0; i < tempList.size(); i++){
        if(tempList[i]->vid.compare(mediaId) == 0){
            //função de inserir no hash
            //TODO - Criar função de inserir dados nos hashs caso não existam lá
            return i;
        }
    }
}
