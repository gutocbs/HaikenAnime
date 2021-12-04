#include "mangamanager.h"

MangaManager::MangaManager(QObject *parent) : MediaManager(parent)
{
}

IMediaManager *MangaManager::getInstance()
{
    return this->instance();
}

MangaManager *MangaManager::createInstance()
{
    return new MangaManager();
}

MangaManager *MangaManager::instance()
{
    return Singleton<MangaManager>::instance(MangaManager::createInstance);
}
