#include "mediainformationcontroller.h"
//TODO - FAZER SINGLETON
MediaInformationController *MediaInformationController::createInstance()
{
    return new MediaInformationController();
}

MediaInformationController *MediaInformationController::instance()
{
    return Singleton<MediaInformationController>::instance(MediaInformationController::createInstance);
}

MediaInformationController::MediaInformationController(QObject *parent) : QObject(parent)
{
    mediaList = Enums::mediaList::CURRENT;
    selectedMediaIndex = 0;
    selectedPage = 1;
    selectedMediaGridIndex = 0;
    mediaController = new MediaController(this);
    mediaListManager = mediaController->instance()->getMediaListManager();
    mediaManager = mediaController->instance()->getMediaManager();
}

void MediaInformationController::getMediaList(QVariant order, QVariant year, bool changeOrder)
{
    mediaListManager = mediaController->instance()->getMediaListManager(mediaType);
    mediaManager = mediaController->instance()->getMediaManager(mediaType);
    if(changeOrder)
        mediaListManager->getInstance()->setListOrder(order);
    mediaListManager->getInstance()->sortList(mediaList, year);
    //Check if the list is empty for some reason
    if(mediaListManager->getInstance()->size(mediaList) < selectedMediaIndex){
        selectedMediaIndex = 0;
        selectedPage = 1;
    }
    getSelectedMediaData(selectedMediaIndex);
}

void MediaInformationController::getSelectedMediaData(QVariant selectedMediaGridIndex)
{
//    emit sdirImagensMedias(QVariant(QDir::currentPath() + QDir::separator() + DirectoriesConfigurationLoader::vdiretorioImagensMedio));
//    emit sdirImagensGrandes(QVariant(QDir::currentPath() + QDir::separator() + DirectoriesConfigurationLoader::vdiretorioImagensGrandes));
    //TODO - Esses sinais deveriam estar em outra funcção da main class
    bool ok;
    this->selectedMediaGridIndex = selectedMediaGridIndex.toInt(&ok);
    if(ok)
        selectedMediaIndex = getPageIndexRange()+this->selectedMediaGridIndex;

    if(mediaListManager->getInstance()->size(mediaList) > selectedMediaIndex){
        QJsonObject selectedMediaObject = MediaUtil::getMediaAsJsonObject(mediaListManager->getInstance()->getMediaByIndex(mediaList, selectedMediaIndex));
        emit signalSelectedMedia(selectedMediaObject);
    }
    getMediaListPage();
}

void MediaInformationController::getMediaListPage()
{
    int mediaListIndex;
    for(int i = 0; i < 12; i++)
    {
        mediaListIndex = (getPageIndexRange())+i;
        if(mediaListManager->getInstance()->size(mediaList) > mediaListIndex)
            emit signalIdMedia(i);
            //GUIController::sendSignal(idMedia, i);
        else
            emit signalIdMedia(i, true);
        //GUIController::sendSignal(idMedia, i, true);
    }
}

int MediaInformationController::getPageIndexRange()
{
    int mediaNumberPerPage{12};
    //-1 since the page starts at 1, not 0
    return mediaNumberPerPage*(selectedPage-1);
}

void MediaInformationController::setPage(int numberPages)
{
    int mediaNumberPerPage{12};
    if((numberPages > 0 && mediaListManager->getInstance()->size(mediaList) > mediaNumberPerPage+(getPageIndexRange())) ||
       (numberPages < 0 && selectedPage > 1)){
        selectedPage += numberPages;
        getMediaListPage();
    }
}

QVariant MediaInformationController::getMediaJsonObjectByGridIndex(QVariant gridIndex)
{
    if(mediaListManager->getInstance()->size(mediaList) > (getPageIndexRange())+gridIndex.toInt())
        return QVariant(MediaUtil::getMediaAsJsonObject(mediaListManager->getInstance()->getMediaByIndex(mediaList, getPageIndexRange()+gridIndex.toInt())));
    return QVariant("");
}







