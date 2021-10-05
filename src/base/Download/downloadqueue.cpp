#include "downloadqueue.h"

DownloadQueue::DownloadQueue(QObject *parent) : QObject(parent)
{
}

void DownloadQueue::insertCoverDownloadQueue(int mediaId, DownloadEnums::imageSize imageSize)
{
    mediaQueue.insert(mediaId, imageSize);
}

void DownloadQueue::downloadMedia()
{
    if(mediaQueue.size() > 0){
        QPointer<DownloadManager> downloadManager = new DownloadManager();
        int maxWorkers = 5;
        if(downloadManager->instance()->getWorkersNumber() > maxWorkers){
            for(int i = 0; i < maxWorkers - downloadManager->instance()->getWorkersNumber(); i++)
                downloadManager->instance()->setWorker();
        }
        foreach(QVariant media, mediaQueue.keys()){
            downloadManager->instance()->work(media);
        }
        mediaQueue.clear();
    }
    //Recursão 30 segundos após finalizar a execução
    QTimer::singleShot(30, this, &DownloadQueue::downloadMedia);
}
