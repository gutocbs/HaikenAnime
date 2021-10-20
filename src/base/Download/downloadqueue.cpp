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
    downloadCovers();
    downloadURLs();
    //Recursão 30 segundos após finalizar a execução
    QTimer::singleShot(30, this, &DownloadQueue::downloadMedia);
}

void DownloadQueue::downloadCovers()
{
    if(mediaQueue.size() > 0 && !isDownloading){
        isDownloading = true;
        QPointer<DownloadManager> downloadManager = new DownloadManager();
        int maxWorkers = 5;
        if(downloadManager->instance()->getWorkersNumber() < maxWorkers){
            for(int i = 0; i < maxWorkers - downloadManager->instance()->getWorkersNumber(); i++)
                downloadManager->instance()->setWorker();
        }
        QHash<int, DownloadEnums::imageSize>::iterator iterator;
        for (iterator = mediaQueue.begin(); iterator != mediaQueue.end(); ++iterator){
            downloadManager->instance()->setWork(iterator.key());
        }
        downloadManager->instance()->work();
        mediaQueue.clear();
        isDownloading = false;
    }
}

void DownloadQueue::downloadURLs()
{
    if(mediaQueue.size() > 0 && !isDownloading){
        isDownloading = true;
        QPointer<DownloadManager> downloadManager = new DownloadManager();
        int maxWorkers = 5;
        if(downloadManager->instance()->getWorkersNumber() > maxWorkers){
            for(int i = 0; i < maxWorkers - downloadManager->instance()->getWorkersNumber(); i++)
                downloadManager->instance()->setWorker();
        }
        QHash<QString, DownloadEnums::imageSize>::iterator iterator;
        for (iterator = mediaQueueURL.begin(); iterator != mediaQueueURL.end(); ++iterator){
            downloadManager->instance()->setWork(iterator.key());
        }
        downloadManager->instance()->work();
        isDownloading = false;
        mediaQueueURL.clear();
    }
}
