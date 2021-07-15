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

    // IMediaManager interface
public:
    QVector<anime *> getList(QString lista, bool seasonal = false, QString argumento = "");
    QVector<anime *> getAnimeList(QString lista, QString argumento);
    QVector<anime *> getMangaList(QString lista, QString argumento);
    QVector<anime *> getNovelList(QString lista, QString argumento);
    QVector<anime *> getAnimeSeasonalList(QString lista);
    QVector<anime *> getSortList(const QString &rordem, QString lista, Enums::mediaType type);
    QVector<anime *> getAnimeYearlyList(int ano);
    QVector<anime *> getSearchList(const QString &rnome, Enums::mediaType rtipoMidia);
    bool compareMedia(QString oficialTitle, QString englishTitle, QStringList alternativeTitles, QString searchedTitle);


    // IMediaManager interface
public:
    void addToHashList(QString mediaId, Enums::mediaList mediaList, Enums::hashList hashList);
};

#endif // ANIMEMANAGER_H
