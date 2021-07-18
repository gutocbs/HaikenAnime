#ifndef IMEDIAMANAGER_H
#define IMEDIAMANAGER_H

#include <QObject>
#include <QHash>
#include <QMetaEnum>
#include "src/utilities/Enums.h"
#include "src/base/anime.h"

class IMediaManager : QObject{
    Q_OBJECT

public:
    virtual QHash<QString, int> getHash(Enums::hashList, int valorInt = 0) = 0;
    virtual QHash<QString, QString> getHash(Enums::hashList, QString valorQString = "") = 0;
    virtual QHash<QString, QStringList> getHash(Enums::hashList, QStringList valorQStringList = QStringList()) = 0;
    //return functions
    virtual QVector<anime*> getMediaList(Enums::mediaList mediaList, QString searchArgument = "") = 0;
    virtual QVector<anime*> getSortList(Enums::mediaOrder order, Enums::orderType orderType, Enums::mediaList mediaList) = 0;

    virtual QVector<anime*> getAnimeYearlyList(int ano) = 0;

    virtual bool compareMedia(QString oficialTitle, QString englishTitle, QStringList alternativeTitles, QString searchedTitle) = 0;
    virtual void addToHashList(QString mediaId, Enums::mediaList mediaList, Enums::hashList hashList) = 0;



protected:
    Enums::mediaType mediaType;
    Enums::mediaList mediaList;
    Enums::hashList hashList;

    QVector<anime*> mediaListCurrent;
    QVector<anime*> mediaListCompleted;
    QVector<anime*> mediaListPaused;
    QVector<anime*> mediaListDropped;
    QVector<anime*> mediaListPlanning;

    QVector<anime*> mediaListSearch;

    QVector<anime*> mediaListSeasonWinter;
    QVector<anime*> mediaListSeasonSummer;
    QVector<anime*> mediaListSeasonFall;
    QVector<anime*> mediaListSeasonSpring;

    QHash<QString,QString> hashMediaListById;
    QHash<QString,int> hashMediaIndexById;
    QHash<QString,QStringList> hashMediaNameById;
    QHash<QString, QStringList> hashMediaCustomNames;
    QHash<int,QString> hashMediaListSizeByYear;
};

#endif // IMEDIAMANAGER_H
