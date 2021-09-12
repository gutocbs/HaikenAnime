#include "downloadworker.h"

DownloadWorker::DownloadWorker(QObject *parent) : QObject(parent)
{

}

void DownloadWorker::getList(Enums::mediaList mediaList, Enums::mediaType mediaType)
{
    QPointer<MediaController> mediaController = new MediaController(this);
    switch(mediaType){
    case Enums::ANIME:
        listManager = mediaController->getMediaListManager(Enums::ANIME);
        break;
    case Enums::MANGA:
        listManager = mediaController->getMediaListManager(Enums::MANGA);
        break;
    case Enums::NOVEL:
        listManager = mediaController->getMediaListManager(Enums::NOVEL);
        break;
    }
    listManager->getMediaList(mediaList);
}

void DownloadWorker::download(fileType fileType, QString url, imageSize imageSize)
{
    QString downloadPath = getDownloadPath(fileType, imageSize);
    downloadPath.append(getFileName(url));
    QPointer<QFile> file;
    file->setFileName(downloadPath);
    qint64 downloadFileSize = getFileSize(url);

    //Remove corrupted files or old files
    if((QFile::exists(downloadPath) && file->size() == 0) || (file->size() != downloadFileSize))
        file->remove();
    //If the file exists and is the same, we don't need to download it again
    else if(file->size() == downloadFileSize)
        return;

    QByteArray fileByteArray = get(url);
    bool save = saveFile(file, fileByteArray);
    if(save)
        setFinishedSignal(fileType, imageSize);
}

//TODO - Fazer função
void DownloadWorker::downloadListCovers(Enums::mediaList mediaList, Enums::mediaType mediaType)
{
    //Pegar vetor de medias
    //Loopar pelo vetor
    //Pegar a url de cada midia e ir baixando
    //Baixar arquivo grande e arquivo pequeno

}

QByteArray DownloadWorker::get(QString url)
{
    QNetworkAccessManager acessManager;
    QNetworkRequest request;
    QNetworkReply* acessReply;
    request.setUrl(QUrl(url));
    acessReply = acessManager.get(request);
    while (!acessReply->isFinished())
    {
        qApp->processEvents();
    }
    QByteArray response_data;
    if(acessReply->isReadable())
        response_data = acessReply->readAll();
    else
        qWarning() << acessReply->errorString();
    if(acessReply->isOpen())
        acessReply->close();
    acessReply->deleteLater();
    return response_data;
}

qint64 DownloadWorker::getFileSize(QString url)
{
    QNetworkAccessManager acessManager;
    QNetworkRequest request;
    QNetworkReply* acessReply;
    request.setUrl(QUrl(url));
    acessReply = acessManager.head(request);
    acessManager.head(request);
    while (!acessReply->isFinished())
    {
        qApp->processEvents();
    }
    QVariant contentLength;
    if(acessReply->isReadable())
        contentLength = acessReply->header(QNetworkRequest::ContentLengthHeader);
    else
        qWarning() << acessReply->errorString();
    if(acessReply->isOpen())
        acessReply->close();
    acessReply->deleteLater();
    return contentLength.toLongLong();

}

bool DownloadWorker::saveFile(QPointer<QFile> file, QByteArray fileByteArray)
{
    file->open(QIODevice::WriteOnly);
    if(file->isOpen() && file->isWritable()){
        file->write(fileByteArray);
        return true;
    }
    else{
        file->remove();
    }
    return false;
}
//TODO - EMITIR SINAIS
void DownloadWorker::setFinishedSignal(fileType fileType, imageSize imageSize)
{
    switch (fileType) {
    case DownloadWorker::Avatar:
        break;
    case DownloadWorker::Cover:
        switch(imageSize){
        case DownloadWorker::Small:
            break;
        case DownloadWorker::Medium:
            break;
        case DownloadWorker::Big:
            break;
        }
        break;
    case DownloadWorker::Torrent:
        break;

    }
}

//Ler configuração pra saber onde salvar
QString DownloadWorker::getDownloadPath(fileType fileType, imageSize imageSize)
{
    switch (fileType) {
    case DownloadWorker::Avatar:
        break;
    case DownloadWorker::Cover:
        switch(imageSize){
        case DownloadWorker::Small:
            break;
        case DownloadWorker::Medium:
            break;
        case DownloadWorker::Big:
            break;
        }
        break;
    case DownloadWorker::Torrent:
        break;
    }
    return "";
}

QString DownloadWorker::getFileName(QString url)
{
    return url.mid(url.lastIndexOf(QChar('.')));
}
