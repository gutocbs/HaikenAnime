#include "medialoader.h"

MediaLoader::MediaLoader(QObject *parent, IMediaListManager *mediaListManager) : QObject(parent)
{
    this->mediaListManager = mediaListManager;
}

//TODO - Refatorar função quando terminar de escrever
//TODO - Organizar arquivos nas pastas certas
//TODO - Mudar dados de Media para usar enums
bool MediaLoader::loadMediaFromFile()
{
    finishedLoading = false;

    //Checa se arquivo pode ser lido
    //MOCK
    QFile jsonMediaListFile(":/Anilist/qrc/Mocks/Anilist/MediaListMock.json");
//    QFile jsonMediaListFile("Configurações/Temp/animeList.txt");
    if(!jsonMediaListFile.open(QIODevice::ReadOnly)) {
           qCritical() << "Could not read file!";
           qCritical() << jsonMediaListFile.errorString();
           return false;
    }
    if(jsonMediaListFile.size() == 0)
        return false;

    QByteArray jsonData = jsonMediaListFile.readAll();
    jsonMediaListFile.close();

    QJsonDocument jsonMediaList = QJsonDocument::fromJson(jsonData);
    QVariantList jsonVariantList = qvariant_cast<QVariantList>(jsonMediaList["data"]["Page"]["mediaList"]);
    QJsonArray mediaList = QJsonArray::fromVariantList(jsonVariantList);
    QHash<QString, QStringList> hashMediaNamesById;

    for(int i = 0; i < mediaList.size(); i++){
        mediaObject = mediaList.takeAt(i).toObject().value("media").toObject();
        Enums::mediaType mediaTypeEnum = Enums::QStringToMediaType(getQStringValueFromKey("format"));
        Enums::mediaList mediaListEnum = Enums::QStringToMediaList(mediaList.takeAt(i).toObject().value("status").toString());
        //TODO - Mudar mediaListManager de acordo com o tipo de media
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
        //TODO - ARRUMAR PROXIMO EPISODIO
        QString nextEpisode = getQStringValueFromKey("nextAiringEpisode");
        if(nextEpisode.contains("?"))
            nextEpisode = "Not aired";
        media->vnumProximoEpisodioLancado = nextEpisode;
        media->vnumEpisodiosAssistidos = getQStringValueFromKey("progress");
        media->vformato = getQStringValueFromKey("format");
//        //TODO - ARRUMAR DATA ESTREIA
//        media->vdataEstreia = getQStringValueFromKey("format");
//        //TODO - ARRUMAR DATA PROXIMO EPISODIO
//        media->vdataEpisodio = getQStringValueFromKey("nextAiringEpisode", "airingAt");
//        media->vdatabaseSite = getQStringValueFromKey("siteUrl");
//        //TODO - ARRUMAR STREAM PROXIMO EPISODIO
//        media->vstream = getQStringValueFromKey("nextAiringEpisode");
//        //TODO - ARRUMAR TEMPORADA
//        media->vtemporada = getNumberValueFromKey("nextAiringEpisode");
        mediaListManager->addMedia(media, mediaListEnum);
    }
    finishedLoading = true;
    return true;

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
