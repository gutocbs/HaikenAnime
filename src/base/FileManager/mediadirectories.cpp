#include "mediadirectories.h"

MediaDirectories::MediaDirectories(QObject *parent) : QObject(parent)
{
    generalDirectories = QStringList("H:/");
    mediaDirectoriesById = QHash<int,QString>();
}

bool MediaDirectories::searchForMediaDirectories()
{
    //TODO - Ler vdiretorioAnimes das configurações
    for(int i = 0; i < generalDirectories.size(); i++){
        QDirIterator folderIterator(generalDirectories[i], QDir::Dirs| QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while(folderIterator.hasNext()){
            QDir folder(folderIterator.next());
            QString mediaName = MediaFile::getNameFromPath(folder.path());
            int idAnime = MediaFile::getMediaIdFromFile(mediaName);
            updateMediaPath(idAnime, folder.path());
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
        updateMediaPath(media->id, path);
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
                    && MediaFile::compareFileToMediaName(media, mediaFile.fileName()))
                    return mediaFileInfo.absoluteFilePath();
        }
    }
    return "";
}

bool MediaDirectories::updateMediaPath(int mediaId, const QString &newDirectoryPath)
{
    QPointer<AnimeManager> mediaManager = new AnimeManager();
    mediaDirectoriesById.insert(mediaId,newDirectoryPath);
    mediaManager->getInstance()->updatePath(mediaId, newDirectoryPath);
    return true;
}

void MediaDirectories::setSearch()
{
    QTimer::singleShot(5, this, &MediaDirectories::searchForMediaDirectories);
    //TODO - Emitir sinal
}
