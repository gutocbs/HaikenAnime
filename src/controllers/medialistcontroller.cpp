#include "medialistcontroller.h"

MediaListController::MediaListController(QObject *parent) : QObject(parent)
{

    mediaInformationController = new MediaInformationController(this);
    mediaController = new MediaController(this);
    mediaListManager = mediaController->instance()->getMediaListManager();
    mediaManager = mediaController->instance()->getMediaManager();
    mediaSearchManager = mediaController->instance()->getMediaSearchManager();
}

void MediaListController::selectListCurrent()
{
    if(mediaList != Enums::mediaList::CURRENT){
        mediaList = Enums::mediaList::CURRENT;
        mediaInformationController->instance()->getMediaList();
    }
}

void MediaListController::selectListCompleted()
{
    if(mediaList != Enums::mediaList::COMPLETED){
        mediaList = Enums::mediaList::COMPLETED;
        mediaInformationController->instance()->getMediaList();
    }
}

void MediaListController::selectListPaused()
{
    if(mediaList != Enums::mediaList::PAUSED){
        mediaList = Enums::mediaList::PAUSED;
        mediaInformationController->instance()->getMediaList();
    }
}

void MediaListController::selectListDropped()
{
    if(mediaList != Enums::mediaList::DROPPED){
        mediaList = Enums::mediaList::DROPPED;
        mediaInformationController->instance()->getMediaList();
    }
}

void MediaListController::selectListPlanning()
{
    if(mediaList != Enums::mediaList::PLANNING){
        mediaList = Enums::mediaList::PLANNING;
        mediaInformationController->instance()->getMediaList();
    }
}

void MediaListController::selectTypeAnime()
{
    if(mediaType != Enums::mediaType::ANIME){
        mediaType = Enums::mediaType::ANIME;
        mediaList = Enums::mediaList::CURRENT;
        setMedia();
        mediaInformationController->instance()->getMediaList();
    }
}

void MediaListController::selectTypeManga()
{
    if(mediaType != Enums::mediaType::MANGA){
        mediaType = Enums::mediaType::MANGA;
        mediaList = Enums::mediaList::CURRENT;
        setMedia();
        mediaInformationController->instance()->getMediaList();
    }
}

void MediaListController::selectTypeNovel()
{
    if(mediaType != Enums::mediaType::NOVEL){
        mediaType = Enums::mediaType::NOVEL;
        mediaList = Enums::mediaList::CURRENT;
        setMedia();
        mediaInformationController->instance()->getMediaList();
    }
}

void MediaListController::setMedia()
{
    mediaListManager = mediaController->instance()->getMediaListManager(mediaType);
    mediaManager = mediaController->instance()->getMediaManager(mediaType);
    mediaSearchManager->setMediaListManager(mediaListManager);
    mediaManager->getInstance()->setMediaListManager(mediaListManager);
    mediaManager->getInstance()->setMediaSearchManager(mediaSearchManager);
}
