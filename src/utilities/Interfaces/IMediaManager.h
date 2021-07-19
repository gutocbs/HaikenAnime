#ifndef IMEDIAMANAGER_H
#define IMEDIAMANAGER_H

#include <QObject>
#include "src/utilities/Enums.h"
#include "src/base/anime.h"
#include "src/utilities/Interfaces/IMediaSearchManager.h"

class IMediaManager : public QObject{
public:
    using QObject::QObject;
    virtual void setMediaSearchManager(IMediaSearchManager* mediaSearchManager);
    virtual void setMediaListManager(IMediaListManager* mediaListManager);
    virtual bool updateMediaList(const QString &mediaId, Enums::mediaList newList) = 0;
    virtual bool updateScore(const QString &mediaId, const QString &newScore) = 0;
    virtual bool updateProgress(const QString &mediaId, const QString &mediaProgress) = 0;
    virtual bool deleteFromList(const QString &mediaId) = 0;
    virtual bool insertCustomName(const QString &mediaId, const QStringList &mediaTitle) = 0;

    //TODO - Funções de get da midia - Ex: Get Name
};
#endif // IMEDIAMANAGER_H
