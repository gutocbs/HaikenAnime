#include "mediadownloader.h"

MediaDownloader::MediaDownloader(QObject *parent) : QObject(parent)
{
    downloader = new Downloader(this);
}

void MediaDownloader::insertDownloadQueue(QString list, Enums::mediaType mediaType, Enums::mediaList mediaList, int listSize)
{
    if(!downloadQueue.contains(list) && !MediaComparer::isSeasonal(list)){
        QPointer<MediaList> mediaListPointer = new MediaList(mediaType,mediaList, listSize);
        downloadQueue.insert(list,mediaListPointer);
    }
}

void MediaDownloader::downloadCoverImages()
{
    int waitRetry = 0;
    if(!downloader->isBusy() && !downloadQueue.isEmpty()){
        //Baixa imagens medias
        downloader->setListAndType(downloadQueue.first()->list, downloadQueue.first()->type);
        for(int i = 0; i < downloadQueue.first()->size; i++){
            downloader->work(i);
        }
        //Baixa imagens grandes
        for(int i = 0; i < downloadQueue.first()->size; i++){
            downloader->workBig(i);
        }
        QPointer<MediaList> mediaList = downloadQueue.take(downloadQueue.firstKey());
        mediaList->deleteLater();
        //Caso já tenha iniciado os downloads mas a fila ainda não está vazia, verifica novamente após um minuto
        if(!downloader->isBusy() && !downloadQueue.isEmpty())
            waitRetry = 1000;
    }
    else if(downloader->isBusy() && !downloadQueue.isEmpty())
        waitRetry = 5000;
    if(waitRetry != 0)
        QTimer::singleShot(waitRetry, this, &MediaDownloader::downloadCoverImages);
}
