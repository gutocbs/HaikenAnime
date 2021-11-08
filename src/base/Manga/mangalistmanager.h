#ifndef MANGALISTMANAGER_H
#define MANGALISTMANAGER_H

#include <QObject>
#include "src/DTO/media.h"
#include "src/utilities/Interfaces/IMediaListManager.h"
#include "src/utilities/Media/mediacomparer.h"
#include "src/utilities/singleton.h"

class MangaListManager : public IMediaListManager
{
    Q_OBJECT
    static MangaListManager *createInstance();
public:
    explicit MangaListManager(QObject *parent = nullptr);
    static MangaListManager *instance();
    IMediaListManager *getInstance();
    ///QHash<Id, index>
    ///Get index position of media in the media vector
    QHash<int, Media *> getHashMediaById();
    void addToHash(QPointer<Media> media);
    Enums::mediaType getMediaType();

    bool addMedia(Media *mediaObject, Enums::mediaList);
    bool removeMedia(Media* media, Enums::mediaList mediaList);
    QPointer<Media> getMediaById(const int &id);
    QVector<Media *> getMediaList(Enums::mediaList mediaList, QVariant searchArgument = "");
    QVector<Media *> getSortList(Enums::mediaList mediaList, QVariant searchArguments = "");
    QVector<Media *> getAnimeYearlyList(const int &year);
    bool setMediaList(Enums::mediaList mediaList, QVector<Media *> media);
    QPointer<Media> getMediaByIndex(Enums::mediaList mediaList, const int &index);
    bool compareMedia(const QString &oficialTitle, const QString &englishTitle, QStringList alternativeTitles, const QString &searchedTitle);
    bool containsMedia(const int &id);
};

#endif // MANGALISTMANAGER_H
