#include "mediamanager.h"

MediaManager::MediaManager(QObject *parent) : QObject(parent)
{

}

void MediaManager::playMediaNextEpisode(Media *media)
{
    QString nextEpisodePath = FileManager::getMediaEpisodePath(media, media->progress+1);
    if(!nextEpisodePath.isEmpty())
        FileManager::openFileOrFolder(nextEpisodePath.toUtf8());
}
