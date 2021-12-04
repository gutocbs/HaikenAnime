#include "mangalistmanager.h"

MangaListManager *MangaListManager::createInstance()
{
    return new MangaListManager();
}

MangaListManager::MangaListManager(QObject *parent) : MediaListManager(parent)
{

}

MangaListManager *MangaListManager::instance()
{
    return Singleton<MangaListManager>::instance(MangaListManager::createInstance);
}

IMediaListManager *MangaListManager::getInstance()
{
    return instance();
}

Enums::mediaType MangaListManager::getMediaType()
{
    return Enums::mediaType::MANGA;
}

