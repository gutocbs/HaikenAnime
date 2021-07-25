#include "mediacontroller.h"

MediaController *MediaController::createInstance()
{
    return new MediaController();
}

MediaController *MediaController::instance()
{
    return Singleton<MediaController>::instance(MediaController::createInstance);
}

MediaController::MediaController(QObject *parent) : QObject(parent)
{

}

void MediaController::initializeMedia()
{
    mediaListManager = new AnimeListManager(this);
    mediaSearchManager = new MediaSearchManager(this);
    mediaManager = new AnimeManager(this);
}

QPointer<IMediaListManager> MediaController::getMediaListManager()
{
    return mediaListManager;
}

QPointer<IMediaSearchManager> MediaController::getMediaSearchManager()
{
    return mediaSearchManager;
}

QPointer<IMediaManager> MediaController::getMediaManager()
{
    return mediaManager;
}
