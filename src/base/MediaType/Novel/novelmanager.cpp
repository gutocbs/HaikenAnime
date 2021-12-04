#include "novelmanager.h"

NovelManager::NovelManager(QObject *parent) : MediaManager(parent)
{
}

IMediaManager *NovelManager::getInstance()
{
    return this->instance();
}

NovelManager *NovelManager::createInstance()
{
    return new NovelManager();
}

NovelManager *NovelManager::instance()
{
    return Singleton<NovelManager>::instance(NovelManager::createInstance);
}
