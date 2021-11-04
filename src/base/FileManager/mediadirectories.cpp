#include "mediadirectories.h"

MediaDirectories::MediaDirectories(QObject *parent) : QObject(parent)
{
    generalDirectories = QStringList();
    mediaDirectoriesById = QHash<int,QString>();
}

bool MediaDirectories::addMediaDirectory(int id, QString path)
{
    mediaDirectoriesById.insert(id, path);
    return true;
}

bool MediaDirectories::searchForMediaDirectories()
{
    QVector<int> mediaToSearch = getMediaPathsToSearch();
    //TODO - Ler vdiretorioAnimes das configurações
    for(int i = 0; i < generalDirectories.size(); i++){
        QDirIterator folderIterator(generalDirectories[i], QDir::Dirs| QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while(folderIterator.hasNext()){
            QFile mediaFile(folderIterator.next());
            QFileInfo mediaFileInfo(mediaFile.fileName());
            if(mediaFileInfo.isFile() && FileManager::fileExtensions.contains(mediaFileInfo.completeSuffix())){
                updateMediaPath(mediaToSearch, mediaFileInfo.path());
            }
        }
    }
    return true;
}

QString MediaDirectories::getMediaFolderPath(Media *media)
{
    QHash<int,QString> mediaFolders = mediaDirectoriesById;
    if(mediaFolders.contains(media->id)){
        QDir mediaDirectory(mediaFolders.value(media->id));
        if(mediaDirectory.exists())
            return mediaFolders.value(media->id);
    }

    QString path = searchForMediaDirectory(media);
    if(!path.isEmpty()){
        addMediaDirectory(media->id, path);
        return path;
    }
    return "";
}

QString MediaDirectories::searchForMediaDirectory(Media *media)
{
    //Começa a iterar a pasta em busca das pastas de animes
    for(int i = 0; i < generalDirectories.size(); i++){
        QDirIterator folderIterator(generalDirectories.at(i), QDir::Files);
        while(folderIterator.hasNext()){
            QFile mediaFile(folderIterator.next());
            QFileInfo mediaFileInfo(mediaFile.fileName());
            //Checa se o que foi encontrado é um arquivo ou uma pasta e, no caso de ser um arquivo, se é um arquivo de vídeo
            if(mediaFileInfo.isFile() && FileManager::fileExtensions.contains(mediaFileInfo.completeSuffix())
                    && FileManager::compareFileToMediaName(media, mediaFile.fileName()))
                    return mediaFileInfo.absoluteFilePath();
        }
    }
    return "";
}

QVector<int> MediaDirectories::getMediaPathsToSearch()
{
    QVector<int> idsPathToSearch;
    QHash<int, QString>::iterator iterator;
    for(iterator = mediaDirectoriesById.begin(); iterator != mediaDirectoriesById.end(); ++iterator){
        QDir directory(mediaDirectoriesById[iterator.key()]);
        if(!directory.exists())
            idsPathToSearch.append(iterator.key());
    }
    return idsPathToSearch;
}

bool MediaDirectories::updateMediaPath(QVector<int> mediaToSearch, QString path)
{
    QPointer<AnimeManager> mediaManager = new AnimeManager();
    int idAnime = MediaUtil::getMediaIdFromFile(path);
    if(mediaToSearch.contains(idAnime)){
        mediaDirectoriesById.insert(idAnime, path);
        mediaToSearch.removeAll(idAnime);
        mediaManager->instance()->updatePath(idAnime, path);
    }
    return true;
}
