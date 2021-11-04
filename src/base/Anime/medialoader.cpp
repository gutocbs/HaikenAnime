#include "medialoader.h"

MediaLoader::MediaLoader(QObject *parent) : QObject(parent)
{
}

//TODO - Organizar arquivos nas pastas certas
//TODO - Passar argumento int pra função, pra ler listas de anos
bool MediaLoader::loadMediaFromFile(bool mock)
{
    QJsonObject mediaObject;
    int listSize = 0;
    QPointer<IMediaListManager> mediaListManager;
    QPointer<MediaController> mediaController = new MediaController();
    QString fileName = getFileName(mock);
    if(!MediaUtil::checkIfFileCanBeOpened(fileName))
        return false;
    QJsonArray mediaList = getMediaListArray(fileName);
    listSize = mediaList.size();
    for(int i = 0; i < listSize; i++){
        mediaObject = mediaList.at(i).toObject().value("media").toObject();
        Enums::mediaType mediaTypeEnum = Enums::QStringToMediaType(getQStringValueFromKey(mediaObject, "format"));
        Enums::mediaList mediaListEnum = Enums::QStringToMediaList(mediaList.at(i).toObject().value("status").toString());
        QPointer<Media> media = getMedia(mediaList, i);
        media->mediaList = mediaListEnum;
        media->format = mediaTypeEnum;
        media->totalEpisodes = getNumberChapters(mediaObject, mediaTypeEnum);
        mediaListManager = mediaController->instance()->getMediaListManager(mediaTypeEnum);
        mediaListManager->addMedia(media, mediaListEnum);
        DownloadQueue::insertCoverDownloadQueue(media->id, DownloadEnums::imageSize::Big);
    }
    return true;
}

QString MediaLoader::getFileName(bool mock)
{
    if(mock)
        return ":/Anilist/qrc/Mocks/Anilist/MediaListMock.json";
    else
        return "Configurações/Temp/animeList.txt";
}

QJsonArray MediaLoader::getMediaListArray(QString fileName)
{
    QFile jsonMediaListFile(fileName);
    jsonMediaListFile.open(QFile::ReadOnly);
    QByteArray jsonData = jsonMediaListFile.readAll();
    jsonMediaListFile.close();

    QJsonDocument jsonMediaList = QJsonDocument::fromJson(jsonData);
    QVariantList jsonVariantList = qvariant_cast<QVariantList>(jsonMediaList["media"]);
    return QJsonArray::fromVariantList(jsonVariantList);
}

QPointer<Media> MediaLoader::getMedia(QJsonArray mediaList, int index)
{
    QJsonObject mediaObject;
    mediaObject = mediaList.at(index).toObject().value("media").toObject();
    QPointer<Media> media(new Media);
    media->id = getNumberValueFromKey(mediaObject, "id");
    media->originalName = getQStringValueFromKey(mediaObject, "title","romaji");
    media->englishName = getQStringValueFromKey(mediaObject, "title", "english");
    media->customNames = getQStringListValuesFromKey(mediaObject, "synonyms");
    media->meanScore = getQStringValueFromKey(mediaObject, "averageScore");
    media->coverURL = getQStringValueFromKey(mediaObject, "coverImage", "large");
    media->personalScore = getQStringValueFromKey(mediaObject, "score");
    media->status = getQStringValueFromKey(mediaObject, "status");
    media->synopsis = getQStringValueFromKey(mediaObject, "description");
    media->yearSeason = getQStringValueFromKey(mediaObject, "season") + " " + getQStringValueFromKey(mediaObject, "startDate", "year");
    media->nextAiringEpisodeDate = getNextEpisode(mediaObject, getQStringValueFromKey(mediaObject, "nextAiringEpisode"));
    media->progress = getNumberValueFromKey(mediaObject, "progress");
    media->startDate = getStartDate(mediaObject, getQStringValueFromKey(mediaObject, "startDate"));
    media->nextAiringEpisodeDate = getNextEpisodeDate(mediaObject, "nextAiringEpisode");
    media->siteURL = getQStringValueFromKey(mediaObject, "siteUrl");
    //        //TODO - FAZER UMA CLASSE CHAMADA MEDIA HELPER QUE IRÁ PEGAR QUAL A TEMPORADA CERTA
//        media->vtemporada = getNumberValueFromKey("nextAiringEpisode");
    return media;
}

QString MediaLoader::getQStringValueFromKey(QJsonObject mediaObject, QString objectName, QString key)
{
    QString value = "-";
    if(mediaObject.contains(objectName) && key.isEmpty())
        value = mediaObject.value(objectName).toString();
    else if(mediaObject.contains(objectName) && mediaObject.value(objectName).toObject().contains(key))
        value = mediaObject.value(objectName).toObject().value(key).toString();
    return value;
}

QStringList MediaLoader::getQStringListValuesFromKey(QJsonObject mediaObject, QString objectName, QString key)
{
    QJsonArray jsonArrayValue;
    QStringList value;
    //Verifica se o objeto possui filhos ou não
    if(mediaObject.contains(objectName) && key.isEmpty())
        jsonArrayValue = QJsonArray::fromVariantList(mediaObject.value(objectName).toArray().toVariantList());
    else if(mediaObject.contains(objectName) && mediaObject.value(objectName).toObject().contains(key))
        jsonArrayValue = QJsonArray::fromVariantList(mediaObject.value(objectName).toObject().value(key).toArray().toVariantList());
    foreach(QJsonValue jsonValue, jsonArrayValue){
        value.append(jsonValue.toString());
    }
    return value;
}

int MediaLoader::getNumberValueFromKey(QJsonObject mediaObject, QString objectName, QString key)
{
    int value = 0;
    if(mediaObject.contains(objectName) && key.isEmpty())
        value = mediaObject.value(objectName).toInt();
    else if(mediaObject.contains(objectName) && mediaObject.value(objectName).toObject().contains(key))
        value = mediaObject.value(objectName).toObject().value(key).toInt();
    return value;
}

int MediaLoader::getNumberChapters(QJsonObject mediaObject, Enums::mediaType mediaType)
{
    int chapterNumber = 0;
    if(mediaType == Enums::ANIME && !mediaObject.value("episodes").isNull())
        chapterNumber = mediaObject.value("episodes").toInt();
    if(mediaType == Enums::NOVEL && !mediaObject.value("volumes").isNull())
        chapterNumber = mediaObject.value("volumes").toInt();
    else if(mediaType == Enums::MANGA && !mediaObject.value("chapters").isNull())
        chapterNumber = mediaObject.value("chapters").toInt();
    return chapterNumber;
}

QDate MediaLoader::getStartDate(QJsonObject mediaObject, QString startDateObject)
{
    QJsonObject startDateJsonObject;
    QString month = "1", year = "2000";
    if(mediaObject.contains(startDateObject) && mediaObject.value(startDateObject).toObject().contains("month")
                                             && mediaObject.value(startDateObject).toObject().contains("year")){
        startDateJsonObject = mediaObject.value(startDateObject).toObject();
        if(!startDateJsonObject.value("month").isNull())
            month = startDateJsonObject.value("month").toString();
        if(!startDateJsonObject.value("year").isNull())
            year = startDateJsonObject.value("year").toString();
    }
    return QDate(year.toInt(), month.toInt(), 1);
}

QString MediaLoader::getNextEpisode(QJsonObject mediaObject, QString nextAiringEpisodeObject)
{
    QString nextAiringEpisode = "Not Airing";
    if(mediaObject.contains(nextAiringEpisodeObject) && !mediaObject.value(nextAiringEpisodeObject).isNull())
        nextAiringEpisode = mediaObject.value(nextAiringEpisodeObject).toString();
    return nextAiringEpisode;
}

QString MediaLoader::getNextEpisodeDate(QJsonObject mediaObject, QString objectName)
{
    QDateTime nextEpisodeDateTime;
    if(mediaObject.contains(objectName) && !mediaObject.value(objectName).toObject().value("airingAt").isNull())
        nextEpisodeDateTime.setSecsSinceEpoch(mediaObject.value(objectName).toObject().value("airingAt").toInt());
    QString nextEpisodeDate = getDayOfTheWeek(nextEpisodeDateTime);
    nextEpisodeDate.append(" ").append(nextEpisodeDateTime.time().toString("hh:mm"));
    return nextEpisodeDate;
}

QString MediaLoader::getDayOfTheWeek(QDateTime secondsSinceEpoch)
{
    int dayOfTheWeek = secondsSinceEpoch.date().dayOfWeek();
    QString dayOfTheWeekString;
    switch (dayOfTheWeek) {
        case 1:
            dayOfTheWeekString = "Monday";
        break;
        case 2:
            dayOfTheWeekString = "Tuesday";
        break;
        case 3:
            dayOfTheWeekString = "Wednesday";
        break;
        case 4:
            dayOfTheWeekString = "Thursday";
        break;
        case 5:
            dayOfTheWeekString = "Friday";
        break;
        case 6:
            dayOfTheWeekString = "Saturday";
        break;
        case 7:
            dayOfTheWeekString = "Sunday";
        break;
    }
    return dayOfTheWeekString;
}

Enums::mediaType MediaLoader::getMediaTypeFromKey(QJsonObject mediaObject, QString key)
{
    QString format;
    if(mediaObject.contains(key))
        format = mediaObject.value(key).toInt();
    if(format.compare("MANGA", Qt::CaseInsensitive) == 0 || format.compare("ONE SHOT", Qt::CaseInsensitive) == 0
                                                         || format.compare("ONE_SHOT", Qt::CaseInsensitive) == 0)
        return Enums::mediaType::MANGA;
    else if(format.compare("NOVEL", Qt::CaseInsensitive) == 0)
        return Enums::mediaType::NOVEL;
    return Enums::mediaType::ANIME;
}

Enums::mediaList MediaLoader::getMediaListFromKey(QJsonObject mediaObject, QString key)
{
    Enums::mediaList mediaList = Enums::CURRENT;
    QString list;
    if(mediaObject.contains(key))
        list = mediaObject.value(key).toInt();
    if(list.compare(Enums::enumMediaListToQString(Enums::CURRENT), Qt::CaseInsensitive) == 0)
        mediaList = Enums::CURRENT;
    else if(list.compare(Enums::enumMediaListToQString(Enums::COMPLETED), Qt::CaseInsensitive) == 0)
        mediaList = Enums::COMPLETED;
    else if(list.compare(Enums::enumMediaListToQString(Enums::PAUSED), Qt::CaseInsensitive) == 0)
        mediaList = Enums::PAUSED;
    else if(list.compare(Enums::enumMediaListToQString(Enums::DROPPED), Qt::CaseInsensitive) == 0)
        mediaList = Enums::DROPPED;
    else if(list.compare(Enums::enumMediaListToQString(Enums::PLANNING), Qt::CaseInsensitive) == 0)
        mediaList = Enums::PLANNING;
    return mediaList;
}

