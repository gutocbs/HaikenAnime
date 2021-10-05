#include "downloadworker.h"

DownloadWorker::DownloadWorker(QObject *parent) : QObject(parent)
{
    isBusyDownloading = false;
}

void DownloadWorker::download(DownloadEnums::fileType fileType, QString url, DownloadEnums::imageSize imageSize)
{
    isBusyDownloading = true;
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

bool DownloadWorker::isBusy()
{
    return isBusyDownloading;
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
void DownloadWorker::setFinishedSignal(DownloadEnums::fileType fileType, DownloadEnums::imageSize imageSize)
{
    switch (fileType) {
    case DownloadEnums::fileType::Avatar:
        break;
    case DownloadEnums::fileType::Cover:
        switch(imageSize){
        case DownloadEnums::imageSize::Small:
            break;
        case DownloadEnums::imageSize::Medium:
            break;
        case DownloadEnums::imageSize::Big:
            break;
        }
        break;
    case DownloadEnums::fileType::Torrent:
        break;

    }
    isBusyDownloading = false;
}

//Ler configuração pra saber onde salvar
QString DownloadWorker::getDownloadPath(DownloadEnums::fileType fileType, DownloadEnums::imageSize imageSize)
{
    switch (fileType) {
    case DownloadEnums::fileType::Avatar:
        break;
    case DownloadEnums::fileType::Cover:
        switch(imageSize){
        case DownloadEnums::imageSize::Small:
            break;
        case DownloadEnums::imageSize::Medium:
            break;
        case DownloadEnums::imageSize::Big:
            break;
        }
        break;
    case DownloadEnums::fileType::Torrent:
        break;
    }
    return "";
}

QString DownloadWorker::getFileName(QString url)
{
    return url.mid(url.lastIndexOf(QChar('.')));
}
