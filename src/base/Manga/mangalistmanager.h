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
    ///QHash<Id, index>
    ///Get index position of media in the media vector
    QPointer<Media> getMediaById(int id);
    QHash<int, Media *> getHashMediaById();
    void addToHash(QPointer<Media> media);
    Enums::mediaType getMediaType();

    QVector<Media*> getMediaList(Enums::mediaList mediaList, QString searchArgument = "");
    QVector<Media*> getSortList(Enums::mediaOrder order, Enums::orderType orderType, Enums::mediaList mediaList);
    QVector<Media*> getAnimeYearlyList(int ano);
    QPointer<Media> getMediaByIndex(Enums::mediaList mediaList, int index);

    bool compareMedia(QString oficialTitle, QString englishTitle, QStringList alternativeTitles, QString searchedTitle);
    bool addMedia(Media *mediaObject, Enums::mediaList);
    bool removeMedia(Media* media, Enums::mediaList mediaList);
};

#endif // MANGALISTMANAGER_H