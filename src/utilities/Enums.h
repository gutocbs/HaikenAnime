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
        if(enumName.compare(enumHashListToQString(hashList::LISTA)) == 0) return hashList::LISTA;
        else if(enumName.compare(enumHashListToQString(hashList::POSICAO)) == 0) return hashList::POSICAO;
        else if(enumName.compare(enumHashListToQString(hashList::NOME)) == 0) return hashList::NOME;
        else if(enumName.compare(enumHashListToQString(hashList::NOMEALTERNATIVO)) == 0) return hashList::NOMEALTERNATIVO;
        return hashList::LISTA;
    }
    static mediaList QStringToMediaList(QString enumName){
        if(enumName.compare(enumMediaListToQString(mediaList::CURRENT)) == 0) return mediaList::CURRENT;
        else if(enumName.compare(enumMediaListToQString(mediaList::COMPLETED)) == 0) return mediaList::COMPLETED;
        else if(enumName.compare(enumMediaListToQString(mediaList::PAUSED)) == 0) return mediaList::PAUSED;
        else if(enumName.compare(enumMediaListToQString(mediaList::DROPPED)) == 0) return mediaList::DROPPED;
        else if(enumName.compare(enumMediaListToQString(mediaList::PLANNING)) == 0) return mediaList::PLANNING;
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
