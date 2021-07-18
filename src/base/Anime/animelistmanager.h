#ifndef ANIMELISTMANAGER_H
#define ANIMELISTMANAGER_H

#include <QObject>
#include "src/utilities/Interfaces/IMediaListManager.h"
#include "src/utilities/formatapalavras.h"

class AnimeListManager : public IMediaListManager
{
    static AnimeListManager *createInstance();
public:
    static AnimeListManager *instance();

    QHash<QString, int> getHashMediaIndexById();
    QHash<QString, QString> getHashMediaListById();
    QHash<QString, QStringList> getHashMediaNamesById(Enums::hashList);
    void addToHash(QString id, int index);
    void addToHash(QString id, QString mediaList);
    void addToHash(QString id, QStringList mediaNames, Enums::hashList hashList);
    void addToHashList(QString mediaId, Enums::mediaList mediaList, Enums::hashList hashList);

    QVector<Media*> getMediaList(Enums::mediaList mediaList, QString searchArgument = "");
    QVector<Media*> getSortList(Enums::mediaOrder order, Enums::orderType orderType, Enums::mediaList mediaList);
    QVector<Media*> getAnimeYearlyList(int ano);

    bool compareMedia(QString oficialTitle, QString englishTitle, QStringList alternativeTitles, QString searchedTitle);
    bool addMedia(Media *mediaObject, Enums::mediaList);
};

#endif // ANIMELISTMANAGER_H
