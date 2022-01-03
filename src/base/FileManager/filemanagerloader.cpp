#include "filemanagerloader.h"

FileManagerLoader::FileManagerLoader(QObject *parent) : QObject(parent)
{

}

///Get the all the media directories from the file
bool FileManagerLoader::loadMediaDirectories(bool mock)
{
    QString fileName = getDirectoriesFileName(mock);
    if(!FileManager::checkIfFileCanBeOpened(fileName))
        return false;
    QJsonArray mediaDirectoriesList = getDirectoriesListArray(fileName);
    int listSize = mediaDirectoriesList.size();
    QJsonObject directoryObject;
    QString directoryPath;
    QDir directory;
    for(int i = 0; i < listSize; i++){
        directoryObject = mediaDirectoriesList.at(i).toObject();
        directoryPath = getPathFromJson(directoryObject);
        directory.cd(directoryPath);
        if(directory.exists(directoryPath) && !directory.isEmpty())
            MediaDirectories::updateMediaPath(getIdFromJson(directoryObject), directoryPath);
    }
    return true;
}

QString FileManagerLoader::getDirectoriesFileName(bool mock)
{
    if(mock)
        return ":/FileManager/qrc/Mocks/FileManager/DirectoriesFileMock.json";
    else
        return "Configurations/Temp/mediaFolders.txt";
}

int FileManagerLoader::getIdFromJson(QJsonObject object)
{
    if(object.contains("mediaId"))
        return object.value("mediaId").toInt();
    return 0;
}

QString FileManagerLoader::getPathFromJson(QJsonObject object)
{
    if(object.contains("directory"))
        return object.value("directory").toString();
    return "";
}

QJsonArray FileManagerLoader::getDirectoriesListArray(QString fileName)
{
    QFile jsonMediaListFile(fileName);
    jsonMediaListFile.open(QFile::ReadOnly);
    QByteArray jsonData = jsonMediaListFile.readAll();
    jsonMediaListFile.close();

    QJsonDocument jsonMediaList = QJsonDocument::fromJson(jsonData);
    QVariantList jsonVariantList = qvariant_cast<QVariantList>(jsonMediaList["directories"]);
    return QJsonArray::fromVariantList(jsonVariantList);
}

bool FileManagerLoader::searchMediaDirectories()
{
    bool loaded = loadMediaDirectories();
    if(loaded){
        emit directoriesLoaded();
        return true;
    }
    return false;
}
