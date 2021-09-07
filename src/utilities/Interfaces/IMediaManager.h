#ifndef IMEDIAMANAGER_H
#define IMEDIAMANAGER_H

#include <QObject>
#include "src/utilities/Enums.h"
#include "src/base/anime.h"
#include "src/utilities/Interfaces/IMediaSearchManager.h"

class IMediaManager : public QObject{
public:
    using QObject::QObject;
    virtual void setMediaSearchManager(IMediaSearchManager* mediaSearchManager) = 0;
    virtual void setMediaListManager(IMediaListManager* mediaListManager) = 0;
    virtual bool updateMediaList(int id, Enums::mediaList newList) = 0;
    virtual bool updateScore(int id, const QString &newScore) = 0;
    virtual bool updateProgress(int id, int mediaProgress) = 0;
    virtual bool deleteFromList(int id) = 0;
    virtual bool insertCustomName(int id, const QStringList &mediaTitle) = 0;

    //TODO - Funções de get da midia - Ex: Get Name
};
#endif // IMEDIAMANAGER_H
