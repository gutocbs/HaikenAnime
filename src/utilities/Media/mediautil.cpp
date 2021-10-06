#include "mediautil.h"

MediaUtil::MediaUtil(QObject *parent) : QObject(parent)
{

}

int MediaUtil::getNumberOfWatchedEpisodes(Media *media)
{

}

int MediaUtil::getMediaIdFromFile(QString fileName)
{
    QPointer<AnimeListManager> mediaList = new AnimeListManager();
    QPointer<IMediaSearchManager> mediaSearch = new MediaSearchManager(mediaList);
    QString mediaName = getMediaNameFromFile(fileName);
    QString mediaSeason = getSeasonFromFile(fileName);
    if(!mediaSeason.isEmpty())
        mediaName.append(QString(" " + mediaSeason));
    int idAnime = mediaSearch->getIdFromMediaTitle(mediaName);
    return idAnime;
}

bool MediaUtil::checkIfFileCanBeOpened(QString filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Could not read file!";
        qCritical() << file.errorString();
        return false;
    }
    if(file.size() == 0)
        return false;
    return true;
}

//TODO - Mudar isso pro MediaUtil
QString MediaUtil::getMediaNameFromFile(QString fileName)
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

QString MediaUtil::getSeasonFromFile(QString fileName)
{

}

int MediaUtil::getMediaEpisodeFromFile(QString fileName)
{
    anitomy::Anitomy lanitomy;
    lanitomy.Parse(fileName.toStdWString());
    const auto& lelements = lanitomy.elements();
    int mediaEpisode = QString::fromStdWString(lelements.get(anitomy::kElementEpisodeNumber)).toInt();
    if(mediaEpisode == 0)
        mediaEpisode = 1;

    return mediaEpisode;
}
