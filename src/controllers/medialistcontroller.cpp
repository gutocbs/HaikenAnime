#include "medialistcontroller.h"

MediaListController::MediaListController(QObject *parent) : QObject(parent)
{

    mediaInformationController = new MediaInformationController(this);
    mediaController = new MediaController(this);
    mediaListManager = mediaController->instance()->getMediaListManager();
    mediaManager = mediaController->instance()->getMediaManager();
    mediaSearchManager = mediaController->instance()->getMediaSearchManager();
}

void MediaListController::setList(Enums::mediaList newMediaList, int data)
{
    if(MediaInformationController::mediaList != newMediaList){
        MediaInformationController::mediaList = newMediaList;
        if(MediaInformationController::mediaList != Enums::mediaList::YEAR)
            mediaInformationController->instance()->getMediaList();
        else
            mediaInformationController->instance()->getMediaList(Enums::mediaOrder::StartDate, data);
    }
}

void MediaListController::selectListCompleted()
{
    if(MediaInformationController::mediaList != Enums::mediaList::COMPLETED){
        MediaInformationController::mediaList = Enums::mediaList::COMPLETED;
        mediaInformationController->instance()->getMediaList();
    }
}

void MediaListController::selectListPaused()
{
    if(MediaInformationController::mediaList != Enums::mediaList::PAUSED){
        MediaInformationController::mediaList = Enums::mediaList::PAUSED;
        mediaInformationController->instance()->getMediaList();
    }
}

void MediaListController::selectListDropped()
{
    if(MediaInformationController::mediaList != Enums::mediaList::DROPPED){
        MediaInformationController::mediaList = Enums::mediaList::DROPPED;
        mediaInformationController->instance()->getMediaList();
    }
}

void MediaListController::selectListPlanning()
{
    if(MediaInformationController::mediaList != Enums::mediaList::PLANNING){
        MediaInformationController::mediaList = Enums::mediaList::PLANNING;
        mediaInformationController->instance()->getMediaList();
    }
}

void MediaListController::setMediaType(Enums::mediaType newMediaType)
{
    if(MediaInformationController::mediaType != newMediaType){
        MediaInformationController::mediaType = newMediaType;
        MediaInformationController::mediaList = Enums::mediaList::CURRENT;
        setMedia();
        mediaInformationController->instance()->getMediaList();
    }
}

void MediaListController::selectTypeManga()
{
    if(MediaInformationController::mediaType != Enums::mediaType::MANGA){
        MediaInformationController::mediaType = Enums::mediaType::MANGA;
        MediaInformationController::mediaList = Enums::mediaList::CURRENT;
        setMedia();
        mediaInformationController->instance()->getMediaList();
    }
}

void MediaListController::selectTypeNovel()
{
    if(MediaInformationController::mediaType != Enums::mediaType::NOVEL){
        MediaInformationController::mediaType = Enums::mediaType::NOVEL;
        MediaInformationController::mediaList = Enums::mediaList::CURRENT;
        setMedia();
        mediaInformationController->instance()->getMediaList();
    }
}

void MediaListController::setMedia()
{
    mediaListManager = mediaController->instance()->getMediaListManager(MediaInformationController::mediaType);
    mediaManager = mediaController->instance()->getMediaManager(MediaInformationController::mediaType);
    mediaSearchManager->setMediaListManager(mediaListManager);
    mediaManager->getInstance()->setMediaListManager(mediaListManager);
    mediaManager->getInstance()->setMediaSearchManager(mediaSearchManager);
}
