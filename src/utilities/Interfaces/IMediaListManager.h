#ifndef IMEDIALISTMANAGER_H
#define IMEDIALISTMANAGER_H

#include <QObject>
#include <QHash>
#include <QMetaEnum>
#include <QPointer>
#include "src/utilities/Enums.h"
#include "src/DTO/media.h"
#include "src/utilities/singleton.h"

class IMediaListManager{
public:
    virtual QHash<QString, int> getHashMediaIndexById() = 0;
    virtual QHash<QString, QString> getHashMediaListById() = 0;
    virtual QHash<QString, QStringList> getHashMediaNamesById(Enums::hashList) = 0;
    virtual void addToHash(QString id, int index) = 0;
    virtual void addToHash(QString id, QString mediaList) = 0;
    virtual void addToHash(QString id, QStringList mediaNames, Enums::hashList hashList) = 0;

    //return functions
    virtual QVector<Media*> getMediaList(Enums::mediaList mediaList, QString searchArgument = "") = 0;
    virtual QVector<Media*> getSortList(Enums::mediaOrder order, Enums::orderType orderType, Enums::mediaList mediaList) = 0;
    virtual QVector<Media*> getAnimeYearlyList(int ano) = 0;
    virtual QPointer<Media> getMediaByIndex(Enums::mediaList mediaList, int index) = 0;
    virtual bool compareMedia(QString oficialTitle, QString englishTitle, QStringList alternativeTitles, QString searchedTitle) = 0;
    virtual void addToHashList(QString mediaId, Enums::mediaList mediaList, Enums::hashList hashList) = 0;
    virtual bool addMedia(Media* mediaObject, Enums::mediaList);
    virtual bool removeMedia(Media* media, Enums::mediaList mediaList);

protected:
    QVector<Media*> mediaListCurrent;
    QVector<Media*> mediaListCompleted;
    QVector<Media*> mediaListPaused;
    QVector<Media*> mediaListDropped;
    QVector<Media*> mediaListPlanning;

    QVector<Media*> mediaListSearch;

    QVector<Media*> mediaListSeasonWinter;
    QVector<Media*> mediaListSeasonSummer;
    QVector<Media*> mediaListSeasonFall;
    QVector<Media*> mediaListSeasonSpring;

    QHash<QString,QString> hashMediaListById;
    QHash<QString,int> hashMediaIndexById;
    QHash<QString,QStringList> hashMediaNameById;
    QHash<QString, QStringList> hashMediaCustomNamesById;
    QHash<int,QString> hashMediaListSizeByYear;
};

#endif // IMEDIALISTMANAGER_H
