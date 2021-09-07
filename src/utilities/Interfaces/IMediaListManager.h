#ifndef IMEDIALISTMANAGER_H
#define IMEDIALISTMANAGER_H

#include <QObject>
#include <QHash>
#include <QMetaEnum>
#include <QPointer>
#include "src/utilities/Enums.h"
#include "src/DTO/media.h"

class IMediaListManager : public QObject{
public:
    //TODO - Transformar todas as funções voids em bool ou int
    using QObject::QObject;
    virtual QPointer<Media> getMediaById(int id) = 0;
    virtual QHash<int, Media*> getHashMediaById() = 0;
    virtual void addToHash(QPointer<Media> media) = 0;

    //return functions
    virtual QVector<Media*> getMediaList(Enums::mediaList mediaList, QString searchArgument = "") = 0;
    virtual QVector<Media*> getSortList(Enums::mediaOrder order, Enums::orderType orderType, Enums::mediaList mediaList) = 0;
    virtual QVector<Media*> getAnimeYearlyList(int ano) = 0;
    virtual QPointer<Media> getMediaByIndex(Enums::mediaList mediaList, int index) = 0;
    virtual bool compareMedia(QString oficialTitle, QString englishTitle, QStringList alternativeTitles, QString searchedTitle) = 0;
    virtual void addToHashList(QString mediaId, Enums::mediaList mediaList, Enums::hashList hashList) = 0;
    virtual bool addMedia(Media* mediaObject, Enums::mediaList) = 0;
    virtual bool removeMedia(Media* media, Enums::mediaList mediaList) = 0;

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

    QHash<int,Media*> hashMediaById;
    QHash<int,QString> hashMediaListSizeByYear;
};

#endif // IMEDIALISTMANAGER_H
