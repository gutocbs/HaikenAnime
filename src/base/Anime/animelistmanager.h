#ifndef ANIMELISTMANAGER_H
#define ANIMELISTMANAGER_H

#include <QObject>
#include "src/DTO/media.h"
#include "src/utilities/Interfaces/IMediaListManager.h"
#include "src/utilities/Media/mediacomparer.h"
#include "src/utilities/singleton.h"

class AnimeListManager : public IMediaListManager
{
    Q_OBJECT
    static AnimeListManager *createInstance();
public:
    explicit AnimeListManager(QObject *parent = nullptr);
    static AnimeListManager *instance();
    IMediaListManager *getInstance();
    ///QHash<Id, index>
    ///Get index position of media in the media vector
    QPointer<Media> getMediaById(int id);
    QHash<int, Media *> getHashMediaById();
    void addToHash(QPointer<Media> media);
    Enums::mediaType getMediaType();

    QVector<Media*> getMediaList(Enums::mediaList mediaList, QString searchArgument = "");
    QVector<Media*> getSortList(Enums::mediaList mediaList);
    QVector<Media*> getAnimeYearlyList(int ano);
    QPointer<Media> getMediaByIndex(Enums::mediaList mediaList, int index);

    bool compareMedia(QString oficialTitle, QString englishTitle, QStringList alternativeTitles, QString searchedTitle);
    bool addMedia(Media *mediaObject, Enums::mediaList);
    bool removeMedia(Media* media, Enums::mediaList mediaList);
};

#endif // ANIMELISTMANAGER_H
