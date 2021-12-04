#ifndef MEDIAMANAGER_H
#define MEDIAMANAGER_H

#include <QObject>
#include "src/utilities/Interfaces/IMediaManager.h"
#include "src/DTO/media.h"
#include "src/base/FileManager/filemanager.h"

class MediaManager : public IMediaManager
{
    Q_OBJECT
public:
    explicit MediaManager(QObject *parent = nullptr);
    IMediaManager *getInstance();
    void setMediaSearchManager(IMediaSearchManager *mediaSearchManager);
    void setMediaListManager(IMediaListManager *mediaListManager);

    bool updateMediaList(int id, Enums::mediaList newList);
    bool updateScore(int id, const QString &newScore);
    bool updateProgress(int id, int mediaProgress);
    bool deleteFromList(int id);
    bool insertCustomName(int id, const QStringList &mediaTitle);
    //TODO - Criar função
    bool updatePath(int id, const QString &mediaPath);


private:
    QPointer<IMediaSearchManager> mediaSearchManager;
    QPointer<IMediaListManager> mediaListManager;
signals:

};

#endif // MEDIAMANAGER_H
