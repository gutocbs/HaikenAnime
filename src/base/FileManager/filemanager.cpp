#include "filemanager.h"

FileManager::FileManager(QObject *parent) : QObject(parent)
{

}

//TODO - Ler um arquivo pra pegar todos os tipos de extensões permitidos e salvar em um vetor
bool FileManager::getFileExtensions()
{
    return false;
}

QString FileManager::getMediaNextEpisodePath(Media *media)
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
                                      && compareFileToMedia(media, mediaFile.fileName())){
                return mediaFile.fileName();
            }
        }
    }
    return "";

}

///Check if media folder is known and, case it is, if exists.
///If doesn't exists or it's not know, search for the folder.
///Returns the folder path if found, otherwise returns an empty QString.
QString FileManager::getMediaFolderPath(Media *media)
{
    QHash<QString,QString> mediaFolders = FileManagerLoader::getMediaDirectories();
    if(mediaFolders.contains(media->vid)){
        QDir mediaDirectory(mediaFolders.value(media->vid));
        if(mediaDirectory.exists())
            return mediaFolders.value(media->vid);
    }

    QString path = searchMediaFolderPath(media);
    if(!path.isEmpty()){
        FileManagerSaver::addMediaDirectory(media->vid, path);
        return path;
    }
    return "";
}

///Check if the file found is the searched media. Checks the name and episode.
bool FileManager::compareFileToMedia(Media *media, QString fileName)
{
    int mediaEpisode = getMediaEpisode(fileName);
    QString mediaName = getMediaName(fileName);
    if((MediaComparer::compareEpisodeNumber(media, mediaEpisode)) &&
        (MediaComparer::compareName(media->vnome,mediaName) || MediaComparer::compareName(media->vnomeIngles,mediaName)
                                                           || MediaComparer::compareName(media->vnomeAlternativo, mediaName)))
        return true;
    return false;
}

QString FileManager::searchMediaFolderPath(Media *media)
{
    QStringList mediaFolders = FileManagerLoader::getDirectories();
    //Começa a iterar a pasta em busca das pastas de animes
    for(int i = 0; i < mediaFolders.size(); i++){
        QDirIterator folderIterator(mediaFolders.at(i), QDir::Files);
        while(folderIterator.hasNext()){
            QFile mediaFile(folderIterator.next());
            QFileInfo mediaFileInfo(mediaFile.fileName());
            //Checa se o que foi encontrado é um arquivo ou uma pasta e, no caso de ser um arquivo, se é um arquivo de vídeo
            if(mediaFileInfo.isFile() && fileExtensions.contains(mediaFileInfo.completeSuffix())
                    && compareFileToMedia(media, mediaFile.fileName()))
                    return mediaFileInfo.absoluteFilePath();
        }
    }
    return "";
}

QString FileManager::getMediaName(QString fileName)
{
    anitomy::Anitomy lanitomy;
    lanitomy.Parse(fileName.toStdWString());
    const auto& lelements = lanitomy.elements();
    QString mediaName = QString::fromStdWString(lelements.get(anitomy::kElementAnimeTitle));
    QString mediaSeason = QString::fromStdWString(lelements.get(anitomy::kElementAnimeSeason));
    if(!mediaSeason.isEmpty())
        mediaName.append(QString(" " + mediaSeason));
    return mediaName;
}

int FileManager::getMediaEpisode(QString fileName)
{
    anitomy::Anitomy lanitomy;
    lanitomy.Parse(fileName.toStdWString());
    const auto& lelements = lanitomy.elements();
    int mediaEpisode = QString::fromStdWString(lelements.get(anitomy::kElementEpisodeNumber)).toInt();
    if(mediaEpisode == 0)
        mediaEpisode = 1;

    return mediaEpisode;

}
