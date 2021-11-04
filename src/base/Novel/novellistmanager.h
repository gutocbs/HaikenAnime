#ifndef NOVELLISTMANAGER_H
#define NOVELLISTMANAGER_H

#include <QObject>
#include "src/DTO/media.h"
#include "src/utilities/Interfaces/IMediaListManager.h"
#include "src/utilities/Media/mediacomparer.h"
#include "src/utilities/singleton.h"

class NovelListManager : public IMediaListManager
{
    Q_OBJECT
    static NovelListManager *createInstance();
public:
    explicit NovelListManager(QObject *parent = nullptr);
    static NovelListManager *instance();
    IMediaListManager *getInstance();
    ///QHash<Id, index>
    ///Get index position of media in the media vector
    QPointer<Media> getMediaById(int id);
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
};

#endif // NOVELLISTMANAGER_H
