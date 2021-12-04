#ifndef MEDIALISTMANAGER_H
#define MEDIALISTMANAGER_H

#include <QObject>
#include "src/utilities/Interfaces/IMediaListManager.h"
#include "src/utilities/Media/mediacomparer.h"
#include "src/DTO/ListOrder.h"

class MediaListManager : public IMediaListManager
{
    Q_OBJECT
public:
    explicit MediaListManager(QObject *parent = nullptr);
    ///QHash<Id, index>
    ///Get index position of media in the media vector
    QPointer<Media> getMediaById(const int &id);
    QHash<int, Media *> getHashMediaById();
    void addToHash(QPointer<Media> media);
    Enums::mediaType getMediaType();

    QVector<Media*> getMediaList(Enums::mediaList mediaList, QVariant searchArgument = "");
    bool sortList(Enums::mediaList mediaList, QVariant searchArgument = "");
    bool setMediaList(Enums::mediaList mediaList, QVector<Media *> media);
    int size(Enums::mediaList mediaList);

    bool compareMedia(const QString &oficialTitle, const QString &englishTitle, QStringList alternativeTitles, const QString &searchedTitle);
    bool addMedia(Media *mediaObject, Enums::mediaList);
    bool removeMedia(Media* media, Enums::mediaList mediaList);
    bool containsMedia(const int &id);
    void setListOrder(QVariant newOrder);

    QVector<Media *> getMediaYearlyList(const int &year);
    QPointer<Media> getMediaByIndex(Enums::mediaList mediaList, const int &index);
};

#endif // MEDIALISTMANAGER_H
