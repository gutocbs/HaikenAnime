#include "animemanager.h"

AnimeManager::AnimeManager(QObject *parent) : MediaManager(parent)
{
}

IMediaManager *AnimeManager::getInstance()
{
    return this->instance();
}

AnimeManager *AnimeManager::createInstance()
{
    return new AnimeManager();
}

AnimeManager *AnimeManager::instance()
{
    return Singleton<AnimeManager>::instance(AnimeManager::createInstance);
}
