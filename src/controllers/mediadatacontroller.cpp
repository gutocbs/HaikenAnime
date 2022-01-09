#include "mediadatacontroller.h"

MediaDataController::MediaDataController(QObject *parent) : QObject(parent)
{
    mediaController = new MediaController(this);
    mediaListManager = mediaController->instance()->getMediaListManager();
    mediaManager = mediaController->instance()->getMediaManager();
    clientManager = new ClientManager(this);
    mediaInformationController = new MediaInformationController(this);
}

void MediaDataController::playNextEpisode()
{
    //    MediaManager::playMediaNextEpisode(mediaListManager->getInstance()->getMediaByIndex(mediaList,selectedMediaIndex));
}

void MediaDataController::openMediaWebpage(QVariant data)
{
    bool ok;
    data.toInt(&ok);
    if(ok){
        QDesktopServices::openUrl(QUrl(mediaListManager->getInstance()->
            getMediaByIndex(MediaInformationController::mediaList,MediaInformationController::selectedMediaIndex)->siteURL));
    }
}

void MediaDataController::openMediaFolder()
{
    FileManager::openFileOrFolder(mediaListManager->getInstance()->
                                  getMediaByIndex(MediaInformationController::mediaList,MediaInformationController::selectedMediaIndex));
}

void MediaDataController::setMediaProgress(int mediaId, int mediaProgress)
{
    mediaManager->getInstance()->updateProgress(mediaId, mediaProgress);
    clientManager->addToUpdateQueue(ClientEnums::PROGRESS, mediaId, mediaProgress);
    mediaInformationController->instance()->getMediaList();
}

void MediaDataController::setMediaCustomName(QVariant data)
{
    if(!data.toString().isEmpty())
        mediaManager->getInstance()->insertCustomName(mediaListManager->getInstance()->
            getMediaByIndex(MediaInformationController::mediaList,MediaInformationController::selectedMediaIndex)->id,
                                                      data.toStringList());
}

void MediaDataController::removeMediaFromList()
{
    QPointer<Media> selectedMedia = mediaListManager->getInstance()->getMediaByIndex(MediaInformationController::mediaList,MediaInformationController::selectedMediaIndex);
    clientManager->addToUpdateQueue(ClientEnums::LIST, selectedMedia->id, Enums::mediaList::NOLIST);
    mediaManager->deleteFromList(selectedMedia->id);
    mediaInformationController->instance()->getMediaList();
}

void MediaDataController::setMediaList(QVariant data)
{
    Enums::mediaList newMediaList = Enums::QStringToMediaList(data.toString());
    //Check if the user is trying to change the entry from list A to list A
    if(MediaInformationController::mediaList == newMediaList)
        return;
    mediaManager->updateMediaList(mediaListManager->getInstance()->getMediaByIndex(MediaInformationController::mediaList,MediaInformationController::selectedMediaIndex)->id, newMediaList);
    clientManager->addToUpdateQueue(ClientEnums::updateType::LIST, mediaListManager->getInstance()->getMediaByIndex(MediaInformationController::mediaList,MediaInformationController::selectedMediaIndex)->id, newMediaList);
    mediaInformationController->instance()->getMediaList();
}
