#include "filemanagerloader.h"

FileManagerLoader::FileManagerLoader(QObject *parent) : MediaDirectories(parent)
{

}

QStringList FileManagerLoader::getDirectories()
{

}

//TODO
//Isso deveria estar na classe MediaDirectories. Aqui só vai dar load nos arquivos
//Também deveria só retornar as pastas, e não procurar por elas. Procurar é outra função
///Search for all media directories. Check if they already exists in the file and search for the ones missing
QHash<int, QString> FileManagerLoader::getMediaDirectories()
{
    anitomy::Anitomy lanitomy;
    QString idAnime;
    QString lfileName;
    QString vtemporada;
    QVector<QString> idsPathToSearch;
    foreach(QString id, mediaDirectoriesById.keys()){
        QDir directory(mediaDirectoriesById[id]);
        if(!directory.exists())
            idsPathToSearch.append(id);
    }
    //Verificar todos os que existem, pra saber se estão vazios.
    //Caso estejam vazios, mudar pra lista dos que nào existem
    //Buscar todos os que não existem
    //Depois de buscar, atualizar o mediaDirectoriesById

//    foreach(checar)

    //Buscar dir 1 por todos os ids
    //Depois dir 2 e assim em diante
    //Busca cada diretorio existente nas configurações
    for(int i = 0; i < vdiretorioAnimes.size(); i++){
        QDirIterator folderIterator(vdiretorioAnimes[i], QDir::Dirs| QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while(folderIterator.hasNext()){
            QFile mediaFile(folderIterator.next());
            QFileInfo mediaFileInfo(mediaFile.fileName());
            if(mediaFileInfo.isFile() && fileExtensions.contains(mediaFileInfo.completeSuffix())){
                //QString mediaName = MediaUtil::GetFileName(mediaFile.fileName())
                //QString mediaSeason = MediaUtil::GetSeason(mediaFile.fileName())
//                    if(!mediaSeason.isEmpty())
//                    mediaName.append(QString(" " + vtemporada));
                //pega o Id disso e verifica se está em idsPathToSearch. Se estiver, confirmado. Se não estiver, ignora.
                mediaDirectoriesById.insert(idAnime, mediaFileInfo.path());
                //Mudar a Media pra colocar o path nela
            }
        }
    }
}

///Get the all the media directories from the file
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
        return ":/FileManager/qrc/Mocks/FileManager/DirectoriesFileMock.json";
    else
        return "Configurações/Temp/animeFolders.txt";
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
