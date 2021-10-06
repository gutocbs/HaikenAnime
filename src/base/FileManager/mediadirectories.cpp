#include "MediaDirectories.h"

MediaDirectories::MediaDirectories(QObject *parent) : QObject(parent)
{
    generalDirectories = QStringList();
    mediaDirectoriesById = QHash<int,QString>();
    fileExtensions = QStringList();
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
            if(mediaFileInfo.isFile() && fileExtensions.contains(mediaFileInfo.completeSuffix())){
                updateMediaPath(mediaToSearch, mediaFileInfo.path());
            }
        }
    }
    return true;
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
