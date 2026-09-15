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

    QHash<QString, int> getHash(Enums::hashList, int valorInt);
    QHash<QString, QString> getHash(Enums::hashList, QString valorQString);
    QHash<QString, QStringList> getHash(Enums::hashList, QStringList valorQStringList);
    void addToHash(QString id, int index);
    void addToHash(QString id, QString mediaList);
    void addToHash(QString id, QStringList mediaNames, Enums::hashList hashList);
    void addToHashList(QString mediaId, Enums::mediaList mediaList, Enums::hashList hashList);

    QVector<anime *> getMediaList(Enums::mediaList mediaList, QString searchArgument = "");
    QVector<anime *> getSortList(Enums::mediaOrder order, Enums::orderType orderType, Enums::mediaList mediaList);
    QVector<anime *> getAnimeYearlyList(int ano);

    bool compareMedia(QString oficialTitle, QString englishTitle, QStringList alternativeTitles, QString searchedTitle);
    bool addMedia(anime *mediaObject, Enums::mediaList);
};

#endif // ANIMELISTMANAGER_H
