#include "animelistmanager.h"

AnimeListManager *AnimeListManager::createInstance()
{
    return new AnimeListManager();
}

AnimeListManager::AnimeListManager(QObject *parent) : MediaListManager(parent)
{

}

AnimeListManager *AnimeListManager::instance()
{
    return Singleton<AnimeListManager>::instance(AnimeListManager::createInstance);
}

IMediaListManager *AnimeListManager::getInstance()
{
    return this->instance();
}

Enums::mediaType AnimeListManager::getMediaType()
{
    return Enums::mediaType::ANIME;
}
