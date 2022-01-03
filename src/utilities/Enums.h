#ifndef ENUMS_H
#define ENUMS_H

#include <QObject>
#include <QMetaEnum>
#include <QVariant>
//OK
class Enums : public QObject{
    Q_OBJECT
public:
    enum hashList{LISTA, POSICAO, NOME, NOMEALTERNATIVO};
    Q_ENUM(hashList)
    enum mediaType{ANIME, MANGA, NOVEL, SEASON = 0, NONE = 0};
    Q_ENUM(mediaType)
    enum mediaList{CURRENT, COMPLETED, PAUSED, DROPPED, PLANNING, SEARCH, WINTER, SUMMER, FALL, SPRING, YEAR, NOLIST = 0};
    Q_ENUM(mediaList)
    enum mediaOrder{Title, StartDate, Progress, Score, Type};
    Q_ENUM(mediaOrder)
    enum orderType{Ascending, Descending};
    Q_ENUM(orderType)
    virtual ~Enums() = 0;

    static hashList QStringToHashList(QString enumName){
        if(enumName.compare(enumHashListToQString(hashList::LISTA), Qt::CaseInsensitive) == 0) return hashList::LISTA;
        else if(enumName.compare(enumHashListToQString(hashList::POSICAO), Qt::CaseInsensitive) == 0) return hashList::POSICAO;
        else if(enumName.compare(enumHashListToQString(hashList::NOME), Qt::CaseInsensitive) == 0) return hashList::NOME;
        else if(enumName.compare(enumHashListToQString(hashList::NOMEALTERNATIVO), Qt::CaseInsensitive) == 0) return hashList::NOMEALTERNATIVO;
        return hashList::LISTA;
    }
    static mediaList QStringToMediaList(QString enumName){
        if(enumName.compare(enumMediaListToQString(mediaList::CURRENT), Qt::CaseInsensitive) == 0) return mediaList::CURRENT;
        else if(enumName.compare(enumMediaListToQString(mediaList::COMPLETED), Qt::CaseInsensitive) == 0) return mediaList::COMPLETED;
        else if(enumName.compare(enumMediaListToQString(mediaList::PAUSED), Qt::CaseInsensitive) == 0) return mediaList::PAUSED;
        else if(enumName.compare(enumMediaListToQString(mediaList::DROPPED), Qt::CaseInsensitive) == 0) return mediaList::DROPPED;
        else if(enumName.compare(enumMediaListToQString(mediaList::PLANNING), Qt::CaseInsensitive) == 0) return mediaList::PLANNING;
        return mediaList::CURRENT;
    }
    static mediaType QStringToMediaType(QString enumName){
        if(enumName.compare(enumMediaTypeToQString(mediaType::ANIME), Qt::CaseInsensitive) == 0) return mediaType::ANIME;
        else if(enumName.compare(enumMediaTypeToQString(mediaType::MANGA), Qt::CaseInsensitive) == 0) return mediaType::MANGA;
        else if(enumName.compare(enumMediaTypeToQString(mediaType::NOVEL), Qt::CaseInsensitive) == 0) return mediaType::NOVEL;
        else if(enumName.compare("ONE SHOT") == 0) return mediaType::MANGA;
        return mediaType::ANIME;
    }
    static mediaOrder QStringToMediaOrder(QString enumName){
        if(enumName.compare(enumMediaOrderToQString(mediaOrder::Title), Qt::CaseInsensitive) == 0) return mediaOrder::Title;
        else if(enumName.compare(enumMediaOrderToQString(mediaOrder::StartDate), Qt::CaseInsensitive) == 0) return mediaOrder::StartDate;
        else if(enumName.compare(enumMediaOrderToQString(mediaOrder::Progress), Qt::CaseInsensitive) == 0) return mediaOrder::Progress;
        else if(enumName.compare(enumMediaOrderToQString(mediaOrder::Score), Qt::CaseInsensitive) == 0) return mediaOrder::Score;
        else if(enumName.compare(enumMediaOrderToQString(mediaOrder::Type), Qt::CaseInsensitive) == 0) return mediaOrder::Type;
        return mediaOrder::Title;
    }
    static QString enumHashListToQString(hashList enumName){
        return QVariant::fromValue(enumName).toString();
    }
    static QString enumMediaListToQString(mediaList enumName){
        return QVariant::fromValue(enumName).toString();
    }
    static QString enumMediaTypeToQString(mediaType enumName){
        return QVariant::fromValue(enumName).toString();
    }
    static QString enumMediaOrderToQString(mediaOrder enumName){
        return QVariant::fromValue(enumName).toString();
    }

};


#endif // ENUMS_H
