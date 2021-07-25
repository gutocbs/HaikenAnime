#ifndef ANIMELISTMANAGER_H
#define ANIMELISTMANAGER_H

#include <QObject>
#include "src/DTO/media.h"
#include "src/utilities/Interfaces/IMediaListManager.h"
#include "src/utilities/formatapalavras.h"

//TODO - Fazer singleton
class AnimeListManager : public IMediaListManager
{
    Q_OBJECT
public:
    explicit AnimeListManager(QObject *parent = nullptr);

    QHash<QString, int> getHashMediaIndexById();
    QHash<QString, QString> getHashMediaListById();
    QHash<QString, QStringList> getHashMediaNamesById(Enums::hashList mediaList);
    void addToHash(QString id, int index);
    void addToHash(QString id, QString mediaList);
    void addToHash(QString id, QStringList mediaNames, Enums::hashList hashList);
    void addToHashList(QString mediaId, Enums::mediaList mediaList, Enums::hashList hashList);

    QVector<Media*> getMediaList(Enums::mediaList mediaList, QString searchArgument = "");
    QVector<Media*> getSortList(Enums::mediaOrder order, Enums::orderType orderType, Enums::mediaList mediaList);
    QVector<Media*> getAnimeYearlyList(int ano);
    QPointer<Media> getMediaByIndex(Enums::mediaList mediaList, int index);

    bool compareMedia(QString oficialTitle, QString englishTitle, QStringList alternativeTitles, QString searchedTitle);
    bool addMedia(Media *mediaObject, Enums::mediaList);
    bool removeMedia(Media* media, Enums::mediaList mediaList);
};

#endif // ANIMELISTMANAGER_H
