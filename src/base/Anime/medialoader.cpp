#include "medialoader.h"

MediaLoader::MediaLoader(QObject *parent, IMediaListManager *mediaListManager) : QObject(parent)
{
    this->mediaListManager = mediaListManager;
}

//TODO - Organizar arquivos nas pastas certas
//TODO - Mudar dados de Media para usar enums
//TODO - Fazer teste para verificar se os hashs tem o mesmo número de entradas que a lista
//TODO - Passar argumento int pra função, pra ler listas de anos
bool MediaLoader::loadMediaFromFile(bool mock)
{
    finishedLoading = false;
    QString fileName = getFileName(mock);
    if(!FileManager::checkIfFileCanBeOpened(fileName))
        return false;
    QJsonArray mediaList = getMediaListArray(fileName);
    listSize = mediaList.size();
    for(int i = 0; i < listSize; i++){
        mediaObject = mediaList.at(i).toObject().value("media").toObject();
        Enums::mediaType mediaTypeEnum = Enums::QStringToMediaType(getQStringValueFromKey("format"));
        Enums::mediaList mediaListEnum = Enums::QStringToMediaList(mediaList.at(i).toObject().value("status").toString());
        QPointer<Media> media = getMedia();
        //TODO - Mudar mediaListManager de acordo com o tipo de media
        mediaListManager->addMedia(media, mediaListEnum);
    }
    finishedLoading = true;
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
    QVariantList jsonVariantList = qvariant_cast<QVariantList>(jsonMediaList["data"]["Page"]["mediaList"]);
    return QJsonArray::fromVariantList(jsonVariantList);
}

QPointer<Media> MediaLoader::getMedia()
{
    QPointer<Media> media(new Media);
    media->vid = QString::number(getNumberValueFromKey("id"));
    media->vnome = getQStringValueFromKey("title","romaji");
    media->vnomeIngles = getQStringValueFromKey("title", "english");
    hashMediaNamesById = mediaListManager->getHashMediaNamesById(Enums::NOMEALTERNATIVO);
    if(!hashMediaNamesById.contains(media->vid))
        hashMediaNamesById.insert(media->vid, getQStringListValuesFromKey("synonyms"));
    media->vnomeAlternativo = hashMediaNamesById[media->vid];
    media->vnotaMediaSite = getQStringValueFromKey("averageScore");
    media->vLinkImagemMedia = getQStringValueFromKey("coverImage", "large");
    media->vnotaMediaPessoal = getQStringValueFromKey("score");
    media->vstatus = getQStringValueFromKey("status");
    media->vsinopse = getQStringValueFromKey("description");
    media->vseason = getQStringValueFromKey("season") + " " + getQStringValueFromKey("startDate", "year");
    media->vnumProximoEpisodioLancado = getNextEpisode(getQStringValueFromKey("nextAiringEpisode"));
    media->vnumEpisodiosAssistidos = getQStringValueFromKey("progress");
    media->vformato = getQStringValueFromKey("format");
     media->vdataEstreia = getStartDate(getQStringValueFromKey("startDate"));
    media->vdataEpisodio = getNextEpisodeDate("nextAiringEpisode");
    media->vdatabaseSite = getQStringValueFromKey("siteUrl");
//        //TODO - FAZER UMA CLASSE CHAMADA MEDIA HELPER QUE IRÁ PEGAR QUAL A TEMPORADA CERTA
//        media->vtemporada = getNumberValueFromKey("nextAiringEpisode");
    return media;
}

QString MediaLoader::getQStringValueFromKey(QString objectName, QString key)
{
    QString value = "-";
    if(mediaObject.contains(objectName) && key.isEmpty())
        value = mediaObject.value(objectName).toString();
    else if(mediaObject.contains(objectName) && mediaObject.value(objectName).toObject().contains(key))
        value = mediaObject.value(objectName).toObject().value(key).toString();
    return value;
}

QStringList MediaLoader::getQStringListValuesFromKey(QString objectName, QString key)
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

int MediaLoader::getNumberValueFromKey(QString objectName, QString key)
{
    int value = 0;
    if(mediaObject.contains(objectName) && key.isEmpty())
        value = mediaObject.value(objectName).toInt();
    else if(mediaObject.contains(objectName) && mediaObject.value(objectName).toObject().contains(key))
        value = mediaObject.value(objectName).toObject().value(key).toInt();
    return value;
}

int MediaLoader::getNumberChapters(Enums::mediaType mediaType)
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

QDate MediaLoader::getStartDate(QString startDateObject)
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

QString MediaLoader::getNextEpisode(QString nextAiringEpisodeObject)
{
    QString nextAiringEpisode = "Not Airing";
    if(mediaObject.contains(nextAiringEpisodeObject) && !mediaObject.value(nextAiringEpisodeObject).isNull())
        nextAiringEpisode = mediaObject.value(nextAiringEpisodeObject).toString();
    return nextAiringEpisode;
}

QString MediaLoader::getNextEpisodeDate(QString objectName)
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
