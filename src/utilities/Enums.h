#ifndef ENUMS_H
#define ENUMS_H

#include <QObject>
#include <QMetaEnum>

class Enums : public QObject{
public:
    enum hashList{LISTA, POSICAO, NOME, NOMEALTERNATIVO};
    Q_ENUM(hashList)
    enum mediaType{ANIME, MANGA, NOVEL, SEASON};
    Q_ENUM(mediaType)
    enum mediaList{CURRENT, COMPLETED, PAUSED, DROPPED, PLANNING, SEARCH, WINTER, SUMMER, FALL, SPRING, YEAR};
    Q_ENUM(mediaList)
    enum mediaOrder{Title, StartDate, Progress, Score, Type};
    Q_ENUM(mediaOrder)
    enum orderType{Ascending, Descending};
    Q_ENUM(orderType)
    virtual ~Enums() = 0;

    static hashList QStringToHashList(QString enumName){
        if(enumName.compare(enumHashListToQString(hashList::LISTA))) return hashList::LISTA;
        else if(enumName.compare(enumHashListToQString(hashList::POSICAO))) return hashList::POSICAO;
        else if(enumName.compare(enumHashListToQString(hashList::NOME))) return hashList::NOME;
        else if(enumName.compare(enumHashListToQString(hashList::NOMEALTERNATIVO))) return hashList::NOMEALTERNATIVO;
        return hashList::LISTA;
    }
    static mediaList QStringToMediaList(QString enumName){
        if(enumName.compare(enumMediaListToQString(mediaList::CURRENT))) return mediaList::CURRENT;
        else if(enumName.compare(enumMediaListToQString(mediaList::COMPLETED))) return mediaList::COMPLETED;
        else if(enumName.compare(enumMediaListToQString(mediaList::PAUSED))) return mediaList::PAUSED;
        else if(enumName.compare(enumMediaListToQString(mediaList::DROPPED))) return mediaList::DROPPED;
        else if(enumName.compare(enumMediaListToQString(mediaList::PLANNING))) return mediaList::PLANNING;
        return mediaList::CURRENT;
    }

    static QString enumHashListToQString(hashList enumName){
        return QMetaEnum::fromType<hashList>().valueToKey(enumName);
    }
    static QString enumMediaListToQString(mediaList enumName){
        return QMetaEnum::fromType<mediaList>().valueToKey(enumName);
    }

};


#endif // ENUMS_H
