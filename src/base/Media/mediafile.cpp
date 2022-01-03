#include "mediafile.h"

MediaFile::MediaFile(QObject *parent) : QObject(parent)
{

}

///Check if the file found is the searched media by the name
bool MediaFile::compareFileToMediaName(Media *media, QString fileName)
{
    QString mediaName = getMediaNameFromFile(fileName);
    if(MediaComparer::compareName(media->originalName,mediaName) || MediaComparer::compareName(media->englishName,mediaName)
                                                          || MediaComparer::compareName(media->customNames, mediaName))
        return true;
    return false;
}

///Check if the file found is the searched media episode.
bool MediaFile::compareFileToMediaEpisode(Media *media, QString fileName, int mediaEpisode)
{
    if(mediaEpisode == 0)
        mediaEpisode = getMediaEpisodeFromFile(fileName);
    if(MediaComparer::compareEpisodeNumber(media, mediaEpisode))
        return true;
    return false;
}


QString MediaFile::getMediaEpisodePath(Media *media, int episode)
{
    QString mediaFolder = media->mediaPath;

    //Verifica se a função retorna um valor que não está vazio, ou seja
    //Se existe uma pasta com o nome do anime
    if(!mediaFolder.isEmpty()){
        //Começa a iterar a pasta em busca das pastas de animes
        QDirIterator folderIterator(mediaFolder, QDirIterator::Subdirectories);
        while(folderIterator.hasNext()){
            QFile mediaFile(folderIterator.next());
            QFileInfo mediaFileInfo(mediaFile.fileName());
            //Checa se o que foi encontrado é um arquivo ou uma pasta e, no caso de ser um arquivo, se é um arquivo de tipo suportado
            if(mediaFileInfo.isFile() && FileManager::fileExtensions.contains(mediaFileInfo.completeSuffix())
                                      && compareFileToMediaName(media, mediaFile.fileName())
                                      && compareFileToMediaEpisode(media, mediaFile.fileName(), episode)){
                return mediaFile.fileName();
            }
        }
    }
    return "";
}

int MediaFile::getMediaIdFromFile(QString fileName)
{
    QPointer<MediaController> mediaController = new MediaController();
    QPointer<IMediaListManager> mediaList = mediaController->instance()->getMediaListManager(Enums::mediaType::ANIME);
    QPointer<IMediaSearchManager> mediaSearch = mediaController->instance()->getMediaSearchManager();
    QString mediaName = getMediaNameFromFile(fileName);
    QString mediaSeason = getSeasonFromFile(fileName);
    if(!mediaSeason.isEmpty())
        mediaName.append(QString(" " + mediaSeason));
    int idAnime = mediaSearch->getIdFromMediaTitle(mediaName);
    return idAnime;
}

int MediaFile::getMediaEpisodeFromFile(QString fileName)
{
    anitomy::Anitomy lanitomy;
    lanitomy.Parse(fileName.toStdWString());
    const auto& lelements = lanitomy.elements();
    int mediaEpisode = QString::fromStdWString(lelements.get(anitomy::kElementEpisodeNumber)).toInt();
    if(mediaEpisode == 0)
        mediaEpisode = 1;

    return mediaEpisode;

}

QString MediaFile::getMediaNameFromFile(QString fileName)
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

QString MediaFile::getSeasonFromFile(QString fileName)
{

}

QString MediaFile::getNameFromPath(QString path)
{
    QFileInfo file(path);
    if(file.exists() && file.isDir())
        return file.baseName();
    else if(file.exists() && file.isFile())
        return file.fileName();
    return "";
}

QString MediaFile::getFolder(QString path)
{
    QFileInfo file(path);
    if(file.exists())
        return file.baseName();
    return "";
}
