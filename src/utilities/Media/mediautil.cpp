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

QJsonObject MediaUtil::getMediaAsJsonObject(Media *media)
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

    QString episodePath = FileManager::getMediaEpisodePath(media, media->progress+1);
    bool hasNextEpisode = false;
    if(!episodePath.isEmpty())
        hasNextEpisode = true;

    QPixmap pixMap;
    QString coverImagePath;
    //TODO - Fazer classe de diretorios padrão
//    if(pixMap.load(cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensGrandes+
//                media->id+".png","png"))
//        coverImagePath = cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensGrandes+
//                                              media->id+".png";
//    else if(pixMap.load(cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensGrandes+
//                     media->id+".jpg", "jpg"))
//        coverImagePath = cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensGrandes+
//                                              media->id+".jpg";
//    else
//        coverImagePath = cconfiguracoesDiretoriosPadrao->instance()->vdiretorioImagensMedio+
//                                 media->id;

    QJsonObject mediaJsonObject{
        {"originalName", media->originalName},
        {"englishName", media->englishName},
        {"synopsis", media->synopsis},
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
        {"id", media->id}
    };
    return mediaJsonObject;
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
