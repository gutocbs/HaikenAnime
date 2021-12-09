#include "mediautil.h"

MediaUtil::MediaUtil(QObject *parent) : QObject(parent)
{

}

int MediaUtil::getTotalEpisodes(Media *media)
{
    int totalEpisodes = media->totalEpisodes;
    if(totalEpisodes == 0)
        totalEpisodes = INT_MAX;
    return totalEpisodes;
}

int MediaUtil::getNumberOfWatchedEpisodes(Media *media)
{

}

QJsonObject MediaUtil::getMediaAsJsonObject(Media *media)
{
    QString mediaNumberReleasedEpisodes = getMediaNumberReleasedEpisodes(media);

//    QString episodePath = FileManager::getMediaEpisodePath(media, media->progress+1);
    bool hasNextEpisode = false;
//    if(!episodePath.isEmpty())
//        hasNextEpisode = true;

    QString coverImagePath;
    QString imgDir = QDir::currentPath() + QDir::separator() + "Configurações" + QDir::separator() + "Imagens" + QDir::separator();
    //TODO - Fazer classe de diretorios padrão
    if(QFile::exists(imgDir + "Grande" + QDir::separator() + QString::number(media->id) +".png"))
        coverImagePath = imgDir + "Grande" + QDir::separator() + QString::number(media->id) +".png";
    else if(QFile::exists(imgDir + "Grande" + QDir::separator() + QString::number(media->id)+".jpg"))
        coverImagePath = imgDir + "Grande" + QDir::separator() + QString::number(media->id)+".jpg";
    else
        coverImagePath = imgDir + "Medio" + QDir::separator() + QString::number(media->id)+".jpg";

    QJsonObject mediaJsonObject{
        {"originalName", media->originalName},
        {"englishName", media->englishName},
        {"status", media->status},
        {"yearSeason", media->yearSeason},
        {"meanScore", QString::number(media->meanScore.toFloat()/10, 'f', 2)},
        {"personalScore", media->personalScore},
        {"nextAiringEpisodeDate", media->nextAiringEpisodeDate},
        {"nextNewEpisode", mediaNumberReleasedEpisodes},
        {"progress", media->progress},
        {"totalEpisodes", media->totalEpisodes},
        {"format", media->format},
        {"hasNextEpisode", hasNextEpisode},
        {"coverImagePath", coverImagePath},
        {"id", media->id},
        {"synopsis", media->synopsis},
        {"list", Enums::enumMediaListToQString(media->mediaList)}
    };
    return mediaJsonObject;
}

QString MediaUtil::getMediaNumberReleasedEpisodes(Media *media)
{
    QString mediaNumberReleasedEpisodes;
    if(media->nextNewEpisode == 0 &&
            media->status != "Finished Airing")
        mediaNumberReleasedEpisodes = "-";
    else if(media->nextNewEpisode == 0)
        mediaNumberReleasedEpisodes = "All Episodes";
    else{
        if(media->nextNewEpisode - 1 == 1)
            mediaNumberReleasedEpisodes = QString::number(media->nextNewEpisode - 1) + " Episode";
        else if(media->nextNewEpisode - 1 == 0)
            mediaNumberReleasedEpisodes = "No Episodes";
        else
            mediaNumberReleasedEpisodes = QString::number(media->nextNewEpisode - 1) + " Episodes";
    }
    return mediaNumberReleasedEpisodes;
}
