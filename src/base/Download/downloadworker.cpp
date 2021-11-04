#include "downloadworker.h"

DownloadWorker::DownloadWorker(QObject *parent) : QObject(parent)
{
    isBusyDownloading = false;
    setImageTypes();
}

void DownloadWorker::download(DownloadEnums::fileType fileType, QString url, DownloadEnums::imageSize imageSize)
{
    isBusyDownloading = true;
    url = getUrlFromImageSize(url, imageSize);
    QString downloadPath = getDownloadPath(fileType, imageSize);
    QString fileName = getFileName(url);
//    qDebug() << "Start " + fileName;
    downloadPath.append(fileName);
    QPointer<QFile> file = new QFile(downloadPath);
    qint64 downloadFileSize = getFileSize(url);

    //Remove corrupted files or old files
    if(QFile::exists(downloadPath) && (file->size() == 0 || file->size() != downloadFileSize))
        file->remove();
    //If the file exists, we don't need to download it again
    if(downloadFileSize != 0 && !QFile::exists(downloadPath)){
        QByteArray fileByteArray = get(url);
        saveFile(file, fileByteArray);
    }
    if(file->isOpen())
        file->close();
    isBusyDownloading = false;
//    qDebug() << "Success " + fileName;
//        setFinishedSignal(fileType, imageSize);
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
    emit downloadFinished();
}

void DownloadWorker::setImageTypes()
{
    //TODO - Ler isso de uma configuração
    imageTypes.append("jpg");
    imageTypes.append("png");
}

//Ler configuração pra saber onde salvar
QString DownloadWorker::getDownloadPath(DownloadEnums::fileType fileType, DownloadEnums::imageSize imageSize)
{
    QString downloadPath = "";
    switch (fileType) {
    case DownloadEnums::fileType::Avatar:
        break;
    case DownloadEnums::fileType::Cover:
        switch(imageSize){
        case DownloadEnums::imageSize::Small:
            break;
        case DownloadEnums::imageSize::Medium:
            downloadPath = QDir::currentPath() + QDir::separator() + "Configurações" + QDir::separator() + "Imagens" + QDir::separator() + "Medio" + QDir::separator();
            break;
        case DownloadEnums::imageSize::Big:
            downloadPath = QDir::currentPath() + QDir::separator() + "Configurações" + QDir::separator() + "Imagens" + QDir::separator() + "Grande" + QDir::separator();
            break;
        }
        break;
    case DownloadEnums::fileType::Torrent:
        break;
    }
    return downloadPath;
}
//TODO - Melhorar função de pegar nome
QString DownloadWorker::getFileName(QString url)
{
    QString fileType = url.mid(url.lastIndexOf(QChar('.'))).remove(".");
    QString fileName = url.mid(url.lastIndexOf(QChar('/'))).remove("/");
    if(imageTypes.contains(fileType)){
        fileName.remove(QRegExp("[A-Za-z]*"));
        fileName = fileName.left(fileName.lastIndexOf(QChar('-')));
    }
    fileName.append(".").append(fileType);
    return fileName;
}

QString DownloadWorker::getUrlFromImageSize(QString url, DownloadEnums::imageSize imageSize)
{
    switch(imageSize){
    case DownloadEnums::imageSize::Small:
        break;
    case DownloadEnums::imageSize::Medium:
        break;
    case DownloadEnums::imageSize::Big:
        url.replace("medium", "large");
        break;
    }
    return url;
}
