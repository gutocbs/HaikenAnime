#ifndef MEDIAMANAGER_H
#define MEDIAMANAGER_H

#include <QObject>
#include "src/DTO/media.h"
#include "src/base/FileManager/filemanager.h"

class MediaManager : public QObject
{
    Q_OBJECT
public:
    explicit MediaManager(QObject *parent = nullptr);
    static void playMediaNextEpisode(Media* media);
signals:

};

#endif // MEDIAMANAGER_H
