#ifndef IMEDIAMANAGER_H
#define IMEDIAMANAGER_H

#include <QObject>
#include "src/utilities/Enums.h"
#include "src/base/anime.h"

class IMediaManager : QObject{
    Q_OBJECT

public:
    virtual QHash<QString, int> getHash(Enums::hashList, int valorInt = 0) = 0;
    virtual QHash<QString, QString> getHash(Enums::hashList, QString valorQString = "") = 0;
    virtual QHash<QString, QStringList> getHash(Enums::hashList, QStringList valorQStringList = QStringList()) = 0;
    //return functions
    virtual QVector<anime*> getList(QString lista, bool seasonal = false, QString searchArgument = "") = 0;
    virtual QVector<anime*> getAnimeList(QString lista, QString argumento = "") = 0;
    virtual QVector<anime*> getMangaList(QString lista, QString argumento = "") = 0;
    virtual QVector<anime*> getNovelList(QString lista, QString argumento = "") = 0;
    virtual QVector<anime*> getAnimeSeasonalList(QString lista) = 0;
    virtual QVector<anime*> getSortList(const QString &rordem, QString lista, Enums::mediaType type) = 0;

    virtual QVector<anime*> getAnimeYearlyList(int ano) = 0;
    virtual QVector<anime*> getSearchList(const QString &rnome, Enums::mediaType rtipoMidia) = 0;

    virtual bool compareMedia(QString oficialTitle, QString englishTitle, QStringList alternativeTitles, QString searchedTitle) = 0;
    virtual void addToHashList(QString mediaId, Enums::mediaList mediaList, Enums::hashList hashList) = 0;



protected:
    Enums::mediaType mediaType;
    Enums::mediaList mediaList;
    Enums::hashList hashList;
};

#endif // IMEDIAMANAGER_H
