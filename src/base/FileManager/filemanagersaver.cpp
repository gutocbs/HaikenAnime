#include "filemanagersaver.h"

FileManagerSaver::FileManagerSaver(QObject *parent) : MediaDirectories(parent)
{

}

bool FileManagerSaver::saveMediaDirectories()
{
    QFile tempFile("Configurações/Temp/mediaFoldersTemp.txt");
    if(!FileManager::checkIfFileCanBeOpened(tempFile.fileName()))
        return false;

    QJsonDocument document = getMediaDirectoriesJsonDocument();
    if(tempFile.open(QIODevice::WriteOnly)){
        tempFile.write(document.toJson());
        tempFile.close();
    }
    if(QFile::exists("Configurações/Temp/mediaFolders.txt")){
        if(QFile::remove("Configurações/Temp/mediaFolders.txt"))
            tempFile.rename("Configurações/Temp/mediaFolders.txt");
    }
    else
        tempFile.rename("Configurações/Temp/mediaFolders.txt");
    return true;
}

QJsonDocument FileManagerSaver::getMediaDirectoriesJsonDocument()
{
    QJsonArray jsonArray;
    QHash<int, QString>::iterator iterator;
    for (iterator = mediaDirectoriesById.begin(); iterator != mediaDirectoriesById.end(); ++iterator){
        QJsonObject obj {
            {"mediaId", iterator.key()},
            {"directory", mediaDirectoriesById[iterator.key()]}
        };
        jsonArray.append(obj);
    }
    QJsonDocument document(jsonArray);
    return document;
}

bool FileManagerSaver::removeAndRenameTempSaveFile()
{
    QFile tempFile("Configurações/Temp/mediaFoldersTemp.txt");
    if(QFile::exists("Configurações/Temp/mediaFolders.txt")){
        if(QFile::remove("Configurações/Temp/mediaFolders.txt"))
            tempFile.rename("Configurações/Temp/mediaFolders.txt");
    }
    else
        tempFile.rename("Configurações/Temp/mediaFolders.txt");
    return true;
}
