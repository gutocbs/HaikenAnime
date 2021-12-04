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
            if(mediaFileInfo.isFile() && FileManager::fileExtensions.contains(mediaFileInfo.completeSuffix())
                                      && compareFileToMediaName(media, mediaFile.fileName())
                                      && compareFileToMediaEpisode(media, mediaFile.fileName(), episode)){
                return mediaFile.fileName();
            }
        }
    }
    return "";

}

//TODO - FAZER FUNÇÃO
QString MediaFile::getMediaFolderPath(Media *media)
{
    return "";
}

int MediaFile::getMediaIdFromFile(QString fileName)
{

}

int MediaFile::getMediaEpisodeFromFile(QString fileName)
{

}

QString MediaFile::getMediaNameFromFile(QString fileName)
{

}

QString MediaFile::getSeasonFromFile(QString fileName)
{

}
