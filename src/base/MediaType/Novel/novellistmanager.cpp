#include "novellistmanager.h"

NovelListManager *NovelListManager::createInstance()
{
    return new NovelListManager();
}

NovelListManager::NovelListManager(QObject *parent) : MediaListManager(parent)
{

}

NovelListManager *NovelListManager::instance()
{
    return Singleton<NovelListManager>::instance(NovelListManager::createInstance);
}


IMediaListManager *NovelListManager::getInstance()
{
    return instance();
}

Enums::mediaType NovelListManager::getMediaType()
{
    return Enums::mediaType::NOVEL;
}

