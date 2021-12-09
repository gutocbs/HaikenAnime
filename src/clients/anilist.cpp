#include "anilist.h"
#include <utility>

const QUrl graphqlUrl("https://graphql.anilist.co");

anilist::anilist(QObject *parent) : IClient(parent)
{
}

anilist::~anilist(){
}

bool anilist::getAvatarAndMediaList(){
    getAvatar();
    getMediaList();
    return true;
}

QNetworkRequest anilist::getRequest(bool auth)
{
    QNetworkRequest request(graphqlUrl);
    if(auth){
        QString token = "Bearer ";
        token.append(vtoken);
        request.setRawHeader(QByteArray("Authorization"), token.toUtf8());
    }
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Accept"), "application/json; charset=utf-8");
    return request;
}

bool anilist::getMediaList()
{
    QFile tempAnimeList("Configurações/Temp/animeList.txt");
    QByteArray mediaJson = getMediaListObject().toJson();

    if(QString(mediaJson).contains("errors")){
        emit downloadFinished(false);
        //TODO - REMOVER ESSES RETURNS 0
        this->thread()->exit(0);
        return false;
    }

    if(tempAnimeList.open(QIODevice::WriteOnly)){
        tempAnimeList.write(mediaJson);
        tempAnimeList.close();
    }

    emit downloadFinished(true);
    getYearlyLists();
    return true;
}

QJsonDocument anilist::getMediaListObject()
{
    QJsonDocument responseDataJson;
    QJsonArray mediaListArray;
    QJsonObject mediaObject;

    QString query = getQuery(AnilistQuery::Avatar);
    query.replace("variableUsername", "gutocbs");
    QByteArray response_data = post(query);
    QJsonDocument resposeJson = QJsonDocument::fromJson(response_data);
    int numberOfPages = resposeJson["data"]["Page"]["pageInfo"]["lastPage"].toInt();

    query = getQuery(AnilistQuery::AnimeInfo);
    for(int i = 1; i < numberOfPages+1; i++){
        QString tempQuery = query;
        tempQuery.replace("variablePage", QString::number(i));
        QByteArray response_data = post(tempQuery);
        QJsonDocument resposeJson = QJsonDocument::fromJson(response_data);
        QJsonArray media = resposeJson["data"]["Page"]["mediaList"].toArray();
        foreach(QJsonValue mediaValue, media){
            mediaListArray.append(mediaValue);
        }
    }
    mediaObject.insert("media", mediaListArray);
    responseDataJson.setObject(mediaObject);
    return responseDataJson;
}

QJsonDocument anilist::getMediaYearListObject(int year)
{
    QJsonDocument responseDataJson;
    QJsonArray mediaListArray;
    QJsonObject mediaObject;
    QString query = getQuery(AnilistQuery::AnimeInfo);
    query.replace("variableAno", QString::number(year));
    QByteArray response_data = post(query);
    QJsonDocument resposeJson = QJsonDocument::fromJson(response_data);
    int numberOfPages = resposeJson["data"]["Page"]["pageInfo"]["lastPage"].toInt();

    query = getQuery(AnilistQuery::YearMediaList);
    for(int i = 1; i < numberOfPages+1; i++){
        QString tempQuery = query;
        tempQuery.replace("variablePage", QString::number(i));
        tempQuery.replace("variableAno", QString::number(year));
        QByteArray response_data = post(tempQuery);
        QJsonDocument resposeJson = QJsonDocument::fromJson(response_data);
        QJsonArray media = resposeJson["data"]["Page"]["mediaList"].toArray();
        foreach(QJsonValue mediaValue, media){
            mediaListArray.append(mediaValue);
        }
    }
    mediaObject.insert("media", mediaListArray);
    responseDataJson.setObject(mediaObject);
    return responseDataJson;
}

QString anilist::getAvatarURL()
{
    QJsonObject mediaObject;
    QString query = getQuery(AnilistQuery::Avatar);
    query.replace("variableUsername", "gutocbs");
    QByteArray responseData = post(query);
    QJsonDocument resposeJson = QJsonDocument::fromJson(responseData);
    mediaObject = resposeJson["data"]["Page"]["mediaList"][0]["user"].toObject();
    QString avatar = mediaObject.value("avatar").toObject().value("large").toString();
    return avatar;
}

QString anilist::getQuery(AnilistQuery query)
{
    QFile queryFile;
    QTextStream textStream(&queryFile);
    switch (query) {
    case anilist::MediaId:
        queryFile.setFileName(":/Anilist/qrc/Anilist/QueryDelete.txt");
        break;
    case YearMediaList:
        queryFile.setFileName(":/Anilist/qrc/Anilist/AnimeInfoAno.txt");
        break;
    case YearLists:
        queryFile.setFileName(":/Anilist/qrc/Anilist/AnoTotalPages.txt");
        break;
    case Avatar:
        queryFile.setFileName(":/Anilist/qrc/Anilist/AvatarTotalPages.txt");
        break;
    case AnimeInfo:
        queryFile.setFileName(":/Anilist/qrc/Anilist/AnimeInfo.txt");
        break;
    }

    if(!queryFile.open(QIODevice::ReadOnly)){
        this->thread()->exit(0);
        return "";
    }

    QString queryString = textStream.readAll();
    queryFile.close();
    queryString.replace("variableUsername", vusername);

    return queryString;
}

QString anilist::getMutationQuery(AnilistMutationType query, int id)
{
    QFile queryFile;
    QTextStream textStream(&queryFile);
    switch (query) {
    case anilist::MutationDelete:
        queryFile.setFileName(":/Anilist/qrc/Anilist/MutationDelete.txt");
        break;
    case anilist::MutationScore:
        queryFile.setFileName(":/Anilist/qrc/Anilist/MutationScore.txt");
        break;
    case anilist::MutationList:
        queryFile.setFileName(":/Anilist/qrc/Anilist/MutationLista.txt");
        break;
    case anilist::MutationProgress:
        queryFile.setFileName(":/Anilist/qrc/Anilist/MutationProgresso.txt");
        break;;
    }

    if(!queryFile.open(QIODevice::ReadOnly)){
        this->thread()->exit(0);
        return "";
    }

    QString queryString = textStream.readAll();
    queryFile.close();
    queryString.replace("variableID", QString::number(id));

    return queryString;
}

QByteArray anilist::post(QString query, bool auth)
{
    QJsonObject jsonObject;
    QNetworkAccessManager acessManager;
    QNetworkRequest request = getRequest(auth);
    QNetworkReply* acessReply;
    jsonObject.insert("query", query.trimmed());
    acessReply = acessManager.post(request, QJsonDocument(jsonObject).toJson());
    while (!acessReply->isFinished())
    {
        qApp->processEvents();
    }
    QByteArray response_data;
    if(acessReply->isReadable())
        response_data = acessReply->readAll();
    else
        qWarning() << acessReply->errorString();
    if(acessReply->isOpen())
        acessReply->close();
    acessReply->deleteLater();
    return response_data;
}

int anilist::getMediaId(int anilistId)
{
    QString query = getQuery(AnilistQuery::MediaId);
    query.replace("variableID", QString::number(anilistId));
    query.replace("variableUsername", vusername);
    QByteArray response_data = post(query);
    QJsonDocument resposeJson = QJsonDocument::fromJson(response_data);
    QString responseString = resposeJson.toJson();
    if(responseString.contains("error"))
        return 0;
    int mediaId = resposeJson["data"]["MediaList"].toObject().value("id").toInt();
    return mediaId;

}

bool anilist::getYearlyLists()
{
    int anilistStartYear = 1998;
    for(int i = 0; i <  QDate::currentDate().year()-anilistStartYear; i++){
        if(!fgetListaAno(2000))
            return false;
    }
    return true;
}

bool anilist::fgetListaAno(int year){
    if(QFile::exists("Configurações/Temp/Lists/animeList"+QString::number(year)+".txt") &&
            (year < QDate::currentDate().year() ||
             //A lista de animes deixa de ser atualizada depois de outubro.
            (year == QDate::currentDate().year() && QDate::currentDate().month() > 10)))
        return true;

    QFile tempAnimeList("Configurações/Temp/Lists/animeList"+QString::number(year)+".txt");
    QByteArray mediaJson = getMediaYearListObject(year).toJson();

    if(QString(mediaJson).contains("errors")){
        emit downloadFinished(false);
        this->thread()->exit(0);
        return false;
    }

    if(tempAnimeList.open(QIODevice::WriteOnly)){
        tempAnimeList.write(mediaJson);
        tempAnimeList.close();
    }
    return true;
}


bool anilist::updateList(int id, Enums::mediaList mediaList){
    QString query = getMutationQuery(AnilistMutationType::MutationList, id);
    query.replace("variableNovaLista", Enums::enumMediaListToQString(mediaList));
    QByteArray response_data = post(query, true);
    QJsonDocument resposeJson = QJsonDocument::fromJson(response_data);
    QString responseString = resposeJson.toJson();
    if(responseString.contains("error"))
        return false;
    return true;
}

bool anilist::updateScore(int id, int novaNota){
    QString query = getMutationQuery(AnilistMutationType::MutationScore, id);
    query.replace("variableScore", QString::number(novaNota));
    QByteArray response_data = post(query, true);
    QJsonDocument resposeJson = QJsonDocument::fromJson(response_data);
    QString responseString = resposeJson.toJson();
    if(responseString.contains("error"))
        return false;
    return true;
}

bool anilist::updateProgress(int mediaId, int mediaProgress){
    QString query = getMutationQuery(AnilistMutationType::MutationProgress, mediaId);
    query.replace("variableProgresso", QString::number(mediaProgress));
    QByteArray response_data = post(query, true);
    QJsonDocument resposeJson = QJsonDocument::fromJson(response_data);
    QString responseString = resposeJson.toJson();
    if(responseString.contains("error"))
        return false;
    return true;
}

QString anilist::getAvatar(){
    return "https://s4.anilist.co/file/anilistcdn/user/"+vavatar;
}

void anilist::setThread(QThread &cThread)
{
//    connect(&cThread, SIGNAL(started()), this, SLOT(fgetList()), Qt::QueuedConnection);

    connect(this, SIGNAL(started()), this, SLOT(fgetList()), Qt::QueuedConnection);
    emit started();
    //    QFuture<void> downloadMedia1 = QtConcurrent::run(this,&anilist::fgetList);
//    fgetList();
}

void anilist::setAuthCode(const QString &ruser, QVariant rauthcode)
{
    vusername = "\"" + ruser + "\"";
    vtoken = rauthcode.toString();
}

bool anilist::deleteMediaFromList(int id){
    int mediaId = getMediaId(id);

    QString query = getMutationQuery(AnilistMutationType::MutationDelete, mediaId);
    QByteArray response_data = post(query, true);
    QJsonDocument resposeJson = QJsonDocument::fromJson(response_data);
    QString responseString = resposeJson.toJson();
    if(responseString.contains("error"))
        return false;
    return true;
}
