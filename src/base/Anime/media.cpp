#include "media.h"

Anime *Anime::createInstance()
{
    return new Anime();
}

Anime *Anime::instance()
{
    return Singleton<Anime>::instance(Anime::createInstance);
}

Anime::Anime(QObject *parent) : QObject(parent)
{

}

void Anime::initializeMedia()
{
    mediaListManager = new AnimeListManager(this);
    mediaSearchManager = new MediaSearchManager(this);
    mediaManager = new AnimeManager(this);
}

QPointer<IMediaListManager> Anime::getMediaListManager()
{
    return mediaListManager;
}

QPointer<IMediaSearchManager> Anime::getMediaSearchManager()
{
    return mediaSearchManager;
}

QPointer<IMediaManager> Anime::getMediaManager()
{
    return mediaManager;
}
