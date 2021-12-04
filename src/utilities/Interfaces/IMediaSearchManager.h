#ifndef IMEDIASEARCHMANAGER_H
#define IMEDIASEARCHMANAGER_H

#include <QObject>
#include <QPointer>
#include <QVector>
#include "src/utilities/Interfaces/IMediaListManager.h"

class IMediaSearchManager : public QObject{
public:
    using QObject::QObject;
    virtual void setMediaListManager(IMediaListManager* mediaListManager) = 0;
    virtual QVector<Media*> searchMedia(const QString &rnome) = 0;
    virtual int buscaIDRapido(const QString &rnomeAnime) = 0;
    virtual int getMediaEpisodeFromId(int id) = 0;
    virtual QString getMediaScoreFromId(int id) = 0;
    virtual QString getMediaTitleFromId(int id) = 0;
    virtual int getIdFromMediaTitle(const QString &mediaTitle) = 0;
    virtual QPointer<Media> fbuscaMediaNoAno(int, int id) = 0;
    virtual QPointer<Media> getMediaFromId(int id) = 0;
    virtual QPointer<Media> getMediaFromListIndex(Enums::mediaList list, int posicao) = 0;
    virtual QVector<Media*> getMediaListFromId(int id) = 0;
    virtual void appendToList(QVector<Media*> &mediaList, Enums::mediaList list, int position) = 0;
    virtual QVector<Media*> getMediaList(const QString &listName) = 0;
    virtual void loadListNames() = 0;
    virtual QStringList getNamesById(int id) = 0;
};

#endif // IMEDIASEARCHMANAGER_H
