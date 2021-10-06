#include "filemanager.h"

FileManager::FileManager(QObject *parent) : MediaDirectories(parent)
{
    getFileExtensions();
}

//TODO - Ler um arquivo pra pegar todos os tipos de extensões permitidos e salvar em um vetor
bool FileManager::getFileExtensions()
{
    return false;
}

QString FileManager::getMediaEpisodePath(Media *media, int episode)
{
    QString mediaFolder = getMediaFolderPath(media);

    //Verifica se a função retorna um valor que não está vazio, ou seja
    //Se existe uma pasta com o nome do anime
    if(!mediaFolder.isEmpty()){
        //Começa a iterar a pasta em busca das pastas de animes
        QDirIterator folderIterator(mediaFolder, QDirIterator::Subdirectories);
        while(folderIterator.hasNext()){
            QFile mediaFile(folderIterator.next());
            QFileInfo mediaFileInfo(mediaFile.fileName());
            //Checa se o que foi encontrado é um arquivo ou uma pasta e, no caso de ser um arquivo, se é um arquivo de tipo suportado
            if(mediaFileInfo.isFile() && fileExtensions.contains(mediaFileInfo.completeSuffix())
                                      && compareFileToMediaName(media, mediaFile.fileName())
                                      && compareFileToMediaEpisode(media, mediaFile.fileName(), episode)){
                return mediaFile.fileName();
            }
        }
    }
    return "";

}

//TODO - Mudar isso pra classe MediaDirectories
///Check if media folder is known and, case it is, if exists.
///Returns the folder path if found, otherwise returns an empty QString.
QString FileManager::getMediaFolderPath(Media *media)
{
    QHash<int,QString> mediaFolders = mediaDirectoriesById;
    if(mediaFolders.contains(media->id)){
        QDir mediaDirectory(mediaFolders.value(media->id));
        if(mediaDirectory.exists())
            return mediaFolders.value(media->id);
    }

    QString path = searchForMediaDirectory(media);
    if(!path.isEmpty()){
        FileManagerSaver::addMediaDirectory(media->id, path);
        return path;
    }
    return "";
}

QString FileManager::searchForMediaDirectory(Media *media)
{
    //Começa a iterar a pasta em busca das pastas de animes
    for(int i = 0; i < generalDirectories.size(); i++){
        QDirIterator folderIterator(generalDirectories.at(i), QDir::Files);
        while(folderIterator.hasNext()){
            QFile mediaFile(folderIterator.next());
            QFileInfo mediaFileInfo(mediaFile.fileName());
            //Checa se o que foi encontrado é um arquivo ou uma pasta e, no caso de ser um arquivo, se é um arquivo de vídeo
            if(mediaFileInfo.isFile() && fileExtensions.contains(mediaFileInfo.completeSuffix())
                    && compareFileToMediaName(media, mediaFile.fileName()))
                    return mediaFileInfo.absoluteFilePath();
        }
    }
    return "";
}

///Check if the file found is the searched media by the name
bool FileManager::compareFileToMediaName(Media *media, QString fileName)
{
    QString mediaName = MediaUtil::getMediaNameFromFile(fileName);
    if(MediaComparer::compareName(media->originalName,mediaName) || MediaComparer::compareName(media->englishName,mediaName)
                                                          || MediaComparer::compareName(media->customNames, mediaName))
        return true;
    return false;
}

///Check if the file found is the searched media episode.
bool FileManager::compareFileToMediaEpisode(Media *media, QString fileName, int mediaEpisode)
{
    if(mediaEpisode == 0)
        mediaEpisode = MediaUtil::getMediaEpisodeFromFile(fileName);
    if(MediaComparer::compareEpisodeNumber(media, mediaEpisode))
        return true;
    return false;
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
