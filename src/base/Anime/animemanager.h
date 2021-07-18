#ifndef ANIMEMANAGER_H
#define ANIMEMANAGER_H

#include <QObject>
#include "src/utilities/Interfaces/IMediaManager.h"

class AnimeManager : public IMediaManager
{

signals:


    // IMediaManager interface
public:
    QHash<QString, int> getHash(Enums::hashList, int valorInt);
    QHash<QString, QString> getHash(Enums::hashList, QString valorQString);
    QHash<QString, QStringList> getHash(Enums::hashList, QStringList valorQStringList);

    QVector<anime *> getMediaList(Enums::mediaList mediaList, QString searchArgument = "");
    QVector<anime *> getSortList(Enums::mediaOrder order, Enums::orderType orderType, Enums::mediaList mediaList);
    QVector<anime *> getAnimeYearlyList(int ano);
    bool compareMedia(QString oficialTitle, QString englishTitle, QStringList alternativeTitles, QString searchedTitle);

    void addToHashList(QString mediaId, Enums::mediaList mediaList, Enums::hashList hashList);
};

#endif // ANIMEMANAGER_H
