#include "filemanager.h"

FileManager::FileManager(QObject *parent) : QObject(parent)
{
    getFileExtensions();
}

//TODO - Ler um arquivo pra pegar todos os tipos de extensões permitidos e salvar em um vetor
bool FileManager::getFileExtensions()
{
    return false;
}

bool FileManager::checkIfFileCanBeOpened(QString filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Could not read file!";
        qCritical() << file.errorString();
        return false;
    }
    if(file.size() == 0)
        return false;
    return true;
}

bool FileManager::openFileOrFolder(const QByteArray &path)
{
    QScopedPointer<QDesktopServices> openFile(new QDesktopServices);
    if(!path.isEmpty()){
        openFile->openUrl(QUrl("file:///"+path,QUrl::TolerantMode));
        return true;
    }
    return false;
}

bool FileManager::openFileOrFolder(Media *media)
{
    QScopedPointer<QDesktopServices> openFile(new QDesktopServices);
    if(!media->mediaPath.isEmpty()){
        openFile->openUrl(QUrl("file:///"+media->mediaPath,QUrl::TolerantMode));
        return true;
    }
    return false;
}
