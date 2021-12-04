#ifndef IMEDIALISTMANAGER_H
#define IMEDIALISTMANAGER_H

#include <QObject>
#include <QHash>
#include <QMetaEnum>
#include <QPointer>
#include <QVariant>
#include "src/DTO/media.h"

class IMediaListManager : public QObject{
public:
    //TODO - Transformar todas as funções voids em bool ou int
    using QObject::QObject;
    virtual IMediaListManager *getInstance() = 0;
    virtual QPointer<Media> getMediaById(const int &id) = 0;
    virtual QHash<int, Media*> getHashMediaById() = 0;
    virtual void addToHash(QPointer<Media> media) = 0;

    //return functions
    virtual QVector<Media*> getMediaList(Enums::mediaList mediaList, QVariant searchArgument = "") = 0;
    virtual bool sortList(Enums::mediaList mediaList, QVariant searchArguments = "") = 0;
    virtual QVector<Media*> getMediaYearlyList(const int &year) = 0;
    virtual int size(Enums::mediaList mediaList) = 0;
    virtual bool setMediaList(Enums::mediaList mediaList, QVector<Media*> media) = 0;
    virtual QPointer<Media> getMediaByIndex(Enums::mediaList mediaList, const int &index) = 0;
    virtual bool compareMedia(const QString &oficialTitle, const QString &englishTitle, QStringList alternativeTitles, const QString &searchedTitle) = 0;
    virtual bool addMedia(Media* mediaObject, Enums::mediaList) = 0;
    virtual bool removeMedia(Media* media, Enums::mediaList mediaList) = 0;
    virtual bool containsMedia(const int &id) = 0;
    virtual Enums::mediaType getMediaType() = 0;
    virtual void setListOrder(QVariant newOrder) = 0;

protected:
    QVector<Media*> mediaList;
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
