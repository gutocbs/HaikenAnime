#include "filemanagerloader.h"

FileManagerLoader::FileManagerLoader(QObject *parent) : MediaDirectories(parent)
{

}

bool FileManagerLoader::loadMediaDirectories(bool mock)
{
    QString fileName = getDirectoriesFileName(mock);
    if(!MediaDirectories::checkIfFileCanBeOpened(fileName))
        return false;
    QJsonArray mediaDirectoriesList = getDirectoriesListArray(fileName);
    int listSize = mediaDirectoriesList.size();
    QJsonObject directoryObject;
    for(int i = 0; i < listSize; i++){
        directoryObject = mediaDirectoriesList.at(i).toObject();
        addMediaDirectory(getIdFromJson(directoryObject), getPathFromJson(directoryObject));
    }
    return true;
}

QString FileManagerLoader::getDirectoriesFileName(bool mock)
{
    if(mock)
        return ":/Anilist/qrc/Mocks/Anilist/MediaListMock.json";
    else
        return "Configurações/Temp/animeList.txt";
}

QString FileManagerLoader::getIdFromJson(QJsonObject object)
{
    if(object.contains("mediaId"))
        return object.value("mediaId").toString();
    return "";
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
