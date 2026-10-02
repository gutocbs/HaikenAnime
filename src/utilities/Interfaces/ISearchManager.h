#ifndef ISEARCHMANAGER_H
#define ISEARCHMANAGER_H

#include <QObject>
#include <QPointer>
#include <QVector>
#include "src/utilities/Enums.h"
#include "src/base/anime.h"

class ISearchManager : QObject{
    Q_OBJECT

public:
    virtual QVector<anime*> searchMedia(const QString &rnome) = 0;
    virtual int getMediaListIndexFromId(const QString &ridAnime) = 0;
    virtual QString buscaIDRapido(const QString &rnomeAnime) = 0;
    virtual QString getMediaListNameFromId(const QString &ridAnime) = 0;
    virtual QString getMediaEpisodeFromId(const QString &rid) = 0;
    virtual QString getMediaScoreFromId(const QString &rid) = 0;
    virtual QString getMediaTitleFromId(const QString &rid) = 0;
    virtual QString getIdFromMediaTitle(const QString &rid) = 0;
    virtual QPointer<anime> fbuscaAnimeNoAno(int, const QString &rid) = 0;
    virtual QPointer<anime> getMediaFromId(const QString &rid) = 0;
    virtual QPointer<anime> getMediaFromListIndex(const QString &rlista, int posicao) = 0;
    virtual QVector<anime*> getMediaListFromId(const QString &rid) = 0;
    virtual void appendToList(QVector<anime*> &mediaList, Enums::mediaList list, int position) = 0;
    virtual QVector<anime*> getMediaList(const QString &listName) = 0;
    virtual void loadListNames() = 0;

protected:
    Enums::mediaType mediaType;
    Enums::mediaList mediaList;
    Enums::hashList hashList;
    QStringList listNames;
};

#endif // ISEARCHMANAGER_H
