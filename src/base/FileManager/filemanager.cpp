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

}
