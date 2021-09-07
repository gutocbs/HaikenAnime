#include "anilist.h"
#include <utility>

const QUrl graphqlUrl("https://graphql.anilist.co");

anilist::anilist(QObject *parent) : QObject(parent)
{
}

anilist::~anilist(){
}

//Sair da thread ao fechar o programa
bool anilist::fgetList(){
    getAvatar();
    getMediaList();
    return true;
}

QNetworkRequest anilist::getRequest()
{
    QNetworkRequest request(graphqlUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Accept"), "application/json; charset=utf-8");
    return request;
}

//TODO - Refatorar
bool anilist::getAvatar()
{
    //Cria o pedido em javascript
    QNetworkRequest request = getRequest();
    QJsonObject json;
    QNetworkAccessManager acessManager;

    //Query que irá solicitar o avatar e o número de páginas que temos que pegar
    QFile avatarTotalPages(":/Anilist/qrc/Anilist/AvatarTotalPages.txt");
    QTextStream textStream(&avatarTotalPages);
    if(!avatarTotalPages.open(QIODevice::ReadOnly)){
        return false;
    }
    QString totalPages = textStream.readAll();
    avatarTotalPages.close();
    totalPages.replace("variableUsername", vusername);
    json.insert("query", totalPages);

    //Post faz o pedido ao servidor request, usando os argumentos em Json
    QNetworkReply* acessReply = acessManager.post(request, QJsonDocument(json).toJson());
    //Espera uma resposta
    //TODO - Colocar um tempo máximo de espera
    while (!acessReply->isFinished())
    {
        qApp->processEvents();
    }

    //Após isso, pegamos a resposta e convertemos em um formato que possamos ler
    QByteArray response_data;
    if(acessReply->isReadable())
        response_data = acessReply->readAll();
    else{
        acessReply->deleteLater();
        return false;
    }
    if(acessReply->isOpen())
        acessReply->close();

    QJsonDocument responseDataJson = QJsonDocument::fromJson(response_data);
    lastPage = responseDataJson.toJson();
    //Verificamos se é uma mensagem de erro
    if(lastPage.contains("errors")){
        emit sterminouDownload(false);
        acessReply->deleteLater();
        return false;
    }
    lastPage = lastPage.toLatin1();
    //Pega avatar
    QString avatarString = lastPage.mid(lastPage.lastIndexOf("avatar"));
    vavatar = avatarString.left(avatarString.indexOf("\"\n"));
    //Pega total de páginas
    avatarString = lastPage.mid(lastPage.lastIndexOf("lastPage")+11);
    lastPage = avatarString.left(avatarString.indexOf(",\n"));

    if(this->thread()->isInterruptionRequested()){
        this->thread()->exit(0);
        acessReply->deleteLater();
        return false;
    }
    return true;
}

bool anilist::getMediaList()
{
    QFile tempAnimeList("Configurações/Temp/animeList.txt");
    QByteArray mediaJson = getMediaListObject().toJson();

    if(QString(mediaJson).contains("errors")){
        emit sterminouDownload(false);
        this->thread()->exit(0);
        return false;
    }

    if(tempAnimeList.open(QIODevice::WriteOnly)){
        tempAnimeList.write(mediaJson);
        tempAnimeList.close();
    }

    emit sterminouDownload(true);
    fgetListasAnoSeason();
    this->thread()->exit(0);
    return true;
}

QJsonDocument anilist::getMediaListObject()
{
    QJsonDocument responseDataJson;
    QJsonArray mediaListArray;
    QJsonObject mediaObject;
    QString query = getQuery(Enums::AnilistQuery::AnimeInfo);
    for(int i = 1; i < lastPage.toInt()+1; i++){
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

QString anilist::getQuery(Enums::AnilistQuery query)
{
    switch (query) {
    case Enums::AnimeInfo:
        QFile animeList(":/Anilist/qrc/Anilist/AnimeInfo.txt");
        QTextStream textStream(&animeList);
        if(!animeList.open(QIODevice::ReadOnly)){
            this->thread()->exit(0);
            return "";
        }
        QString query = textStream.readAll();
        animeList.close();
        query.replace("variableUsername", vusername);
        return query;
        break;
    }
    return "";
}

QByteArray anilist::post(QString query)
{
    QJsonObject jsonObject;
    QNetworkAccessManager acessManager;
    QNetworkRequest request = getRequest();
    QNetworkReply* acessReply;
    jsonObject.insert("query", query.trimmed());
    acessReply = acessManager.post(request, QJsonDocument(jsonObject).toJson());
    while (!acessReply->isFinished())
    {
        qApp->processEvents();
    }
    QByteArray response_data = acessReply->readAll();
    if(acessReply->isOpen())
    {
        acessReply->close();
    }
    acessReply->deleteLater();
    return response_data;
}

bool anilist::fgetListasAnoSeason()
{
    for(int i = 0; i <  QDate::currentDate().year()-1998; i++){
        if(!fgetListaAno(QString::number(2000+i)))
            return false;
    }
    return true;
}

bool anilist::fgetListaAno(const QString &rano){
    if(QFile::exists("Configurações/Temp/Lists/animeList"+rano+".txt") && rano.toInt() < QDate::currentDate().year())
        return true;
    else if(QFile::exists("Configurações/Temp/Lists/animeList"+rano+".txt") && rano.toInt() == QDate::currentDate().year() && QDate::currentDate().month() > 10)
        return true;
    //Cria o pedido em javascript
    QNetworkRequest request(graphqlUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Accept"), "application/json; charset=utf-8");
    QJsonObject json;
    QNetworkAccessManager lacessManager;

    //Query que irá solicitar o avatar e o número de páginas que temos que pegar
    QFile totalPages(":/Anilist/qrc/Anilist/AnoTotalPages.txt");
    QTextStream textStream(&totalPages);
    if(!totalPages.open(QIODevice::ReadOnly)){
        this->thread()->exit(0);
        return false;
    }
    QString query = textStream.readAll();
    totalPages.close();
    query.replace("variableAno", rano);
    json.insert("query", query);

    //Checa se a thread está sendo interrompida, ou seja, se o programa está sendo fechado durante a execução da função
    //Isso vai ocorrer em diversos pontos da thread por conta dos loops
    if(this->thread()->isInterruptionRequested()){
        this->thread()->exit(0);
        return false;
    }

    //Post faz o pedido ao servidor request, usando os argumentos em Json
//    QPointer<QNetworkReply> vreply = lacessManager.post(request, QJsonDocument(json).toJson());
    QNetworkReply* vreply = lacessManager.post(request, QJsonDocument(json).toJson());
    //Espera uma resposta
    while (!vreply->isFinished())
    {
        qApp->processEvents();
    }

    //Após isso, pegamos a resposta e convertemos em um formato que possamos ler
    QByteArray response_data;
    if(vreply->isReadable())
        response_data = vreply->readAll();
    else{
        vreply->deleteLater();
        return false;
    }
    if(vreply->isOpen())
    {
        vreply->close();
    }
    QJsonDocument jsond = QJsonDocument::fromJson(response_data);
    QString lastPage = jsond.toJson();
    //Verificamos se é uma mensagem de erro
    if(lastPage.contains("errors")){
        this->thread()->exit(0);
        vreply->deleteLater();
        return false;
    }
    lastPage = lastPage.toLatin1();
    //Pega avatar
    QString llastPage = lastPage.mid(lastPage.lastIndexOf("avatar"));
    vavatar = llastPage.left(llastPage.indexOf("\"\n"));
    //Pega total de páginas
    llastPage = lastPage.mid(lastPage.lastIndexOf("lastPage")+11);
    lastPage = llastPage.left(llastPage.indexOf(",\n"));

    QFile t("Configurações/Temp/Lists/animeList"+rano+"Temp.txt");
    if(t.open(QIODevice::WriteOnly)){
        for(int i = 1; i < lastPage.toInt()+1; i++){
            QFile animeList(":/Anilist/qrc/Anilist/AnimeInfoAno.txt");
            QTextStream textStream(&animeList);
            if(!animeList.open(QIODevice::ReadOnly)){
                this->thread()->exit(0);
                return false;
            }
            query = textStream.readAll();
            animeList.close();
            query.replace("variablePagina", QString::number(i));
            query.replace("variableAno", rano);
            json.insert("query", query.trimmed());
            vreply = lacessManager.post(request, QJsonDocument(json).toJson());
            while (!vreply->isFinished())
            {
                qApp->processEvents();
            }
            QByteArray response_data = vreply->readAll();
            if(vreply->isOpen())
            {
                vreply->close();
            }
            jsond = QJsonDocument::fromJson(response_data);
            t.write(jsond.toJson());
        }
        t.close();
    }
    QString lreplyString = jsond.toJson();
    if(lreplyString.contains("errors")){
        vreply->deleteLater();
        return false;
    }
    if(QFile::exists("Configurações/Temp/Lists/animeList"+rano+".txt")){
        if(QFile::remove("Configurações/Temp/Lists/animeList"+rano+".txt"))
            t.rename("Configurações/Temp/Lists/animeList"+rano+".txt");
    }
    else
        t.rename("Configurações/Temp/Lists/animeList"+rano+".txt");
    vreply->deleteLater();
    return true;
}


bool anilist::fmudaLista(int rid, const QString &rNovaLista){
    ///Preciso por o token em um arquvio de configuração criptografado
    QString auth = "Bearer ";
    auth.append(vtoken);

    QNetworkRequest request(graphqlUrl);
    request.setRawHeader(QByteArray("Authorization"), auth.toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Accept"), "application/json; charset=utf-8");
    QJsonObject json;
    QNetworkAccessManager lacessManager;

    //Pra deletar
    //QString lnovaLista = "mutation{     DeleteMediaListEntry (mediaId: " + lnovoId + ", status: " + rnovaLista + ") {         id         status     } }";
    //Cria string com a lista nova lista
    QFile animeList(":/Anilist/qrc/Anilist/MutationLista.txt");
    QTextStream textStream(&animeList);
    if(!animeList.open(QIODevice::ReadOnly)){
        this->thread()->exit(0);
        return false;
    }
    QString query = textStream.readAll();
    animeList.close();
    query.replace("variableID", QString::number(rid));
    query.replace("variableNovaLista", rNovaLista);
    //Insere item no json
    json.insert("query", query);
    //Manda a solicitação de mudança
    QPointer<QNetworkReply> vreply = lacessManager.post(request, QJsonDocument(json).toJson());
    //Espera solicitação voltar do servidor
    while (!vreply->isFinished())
    {
        qApp->processEvents();
    }
    QByteArray response_data;
    if(vreply->isReadable())
        response_data = vreply->readAll();
    else{
        qWarning() << vreply->errorString();
        return false;
    }
    if(vreply->isOpen())
    {
        vreply->close();
    }
    QJsonDocument jsond = QJsonDocument::fromJson(response_data);
    QString lreplyString = jsond.toJson();
    if(lreplyString.contains("error"))
        return false;
    return true;
}

///fmudaNota(id, nova nota)
bool anilist::fmudaNota(int rid, int rnovaNota){
    QString auth = "Bearer ";
    auth.append(vtoken);

    //Como pegar o nome ou id de usuário pelo token?
    //Posso pegar igual fazia antes,/ sem o token?
    QNetworkRequest request(graphqlUrl);
    request.setRawHeader(QByteArray("Authorization"), auth.toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Accept"), "application/json; charset=utf-8");
    QJsonObject json;
    QNetworkAccessManager lacessManager;

    //Pra deletar
    //QString lnovaLista = "mutation{     DeleteMediaListEntry (mediaId: " + lnovoId + ", status: " + rnovaLista + ") {         id         status     } }";
    //Cria string com a nova nota
    QFile animeList(":/Anilist/qrc/Anilist/MutationScore.txt");
    QTextStream textStream(&animeList);
    if(!animeList.open(QIODevice::ReadOnly)){
        this->thread()->exit(0);
        return false;
    }
    QString query = textStream.readAll();
    animeList.close();
    query.replace("variableID", QString::number(rid));
    query.replace("variableScore", QString::number(rnovaNota));
    //Insere item no json
    json.insert("query", query);
    //Manda a solicitação de mudança
    QPointer<QNetworkReply> vreply = lacessManager.post(request, QJsonDocument(json).toJson());


    //Espera solicitação voltar do servidor
    while (!vreply->isFinished())
    {
        qApp->processEvents();
    }
    QByteArray response_data;
    if(vreply->isReadable())
        response_data = vreply->readAll();
    else{
        qWarning() << vreply->errorString();
        return false;
    }
    if(vreply->isOpen())
    {
        vreply->close();
    }
    QJsonDocument jsond = QJsonDocument::fromJson(response_data);
    QString lreplyString = jsond.toJson();
    if(lreplyString.contains("error"))
        return false;
    return true;
}

bool anilist::fmudaProgresso(int rid, int rnovoProgresso){
    QString auth = "Bearer ";
    auth.append(vtoken);

    //Como pegar o nome ou id de usuário pelo token?
    //Posso pegar igual fazia antes,/ sem o token?
    QNetworkRequest request(graphqlUrl);
    request.setRawHeader(QByteArray("Authorization"), auth.toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Accept"), "application/json; charset=utf-8");
    QJsonObject json;
    QNetworkAccessManager lacessManager;

    //Pra deletar
    //QString lnovaLista = "mutation{     DeleteMediaListEntry (mediaId: " + lnovoId + ", status: " + rnovaLista + ") {         id         status     } }";
    //Cria string com a nova nota
    QFile animeList(":/Anilist/qrc/Anilist/MutationProgresso.txt");
    QTextStream textStream(&animeList);
    if(!animeList.open(QIODevice::ReadOnly)){
        this->thread()->exit(0);
        return false;
    }
    QString query = textStream.readAll();
    animeList.close();
    query.replace("variableID", QString::number(rid));
    query.replace("variableProgresso", QString::number(rnovoProgresso));
    //Insere item no json
    json.insert("query", query);
    //Manda a solicitação de mudança
    QPointer<QNetworkReply> vreply = lacessManager.post(request, QJsonDocument(json).toJson());

    //Espera solicitação voltar do servidor
    while (!vreply->isFinished())
    {
        qApp->processEvents();
    }
    QByteArray response_data;
    if(vreply->isReadable())
        response_data = vreply->readAll();
    else{
        qWarning() << vreply->errorString();
        return false;
    }
    if(vreply->isOpen())
    {
        vreply->close();
    }
    QJsonDocument jsond = QJsonDocument::fromJson(response_data);
    QString lreplyString = jsond.toJson();
    if(lreplyString.contains("error"))
        return false;
    return true;
}

QString anilist::fretornaAvatar(){
    return "https://s4.anilist.co/file/anilistcdn/user/"+vavatar;
}

void anilist::fbaixaListaThread(QThread &cThread)
{
    connect(&cThread, SIGNAL(started()), this, SLOT(fgetList()), Qt::QueuedConnection);
}

void anilist::frecebeAutorizacao(const QString &ruser, QVariant rauthcode)
{
    vusername = "\"" + ruser + "\"";
    vtoken = rauthcode.toString();
}

bool anilist::fexcluiAnime(int rid){
    QString auth = "Bearer ";
    auth.append(vtoken);

    QNetworkRequest request(graphqlUrl);
    request.setRawHeader(QByteArray("Authorization"), auth.toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Accept"), "application/json; charset=utf-8");
    QJsonObject json;
    QNetworkAccessManager lacessManager;

    //Cria string com o pedido de delete
    QFile animeList(":/Anilist/qrc/Anilist/QueryDelete.txt");
    QTextStream textStream(&animeList);
    if(!animeList.open(QIODevice::ReadOnly)){
        this->thread()->exit(0);
        return false;
    }
    QString query = textStream.readAll();
    animeList.close();
    query.replace("variableID", QString::number(rid));
    query.replace("variableUsername", vusername);
    //Insere item no json
    json.insert("query", query);
    //Manda a solicitação de mudança
    QPointer<QNetworkReply> vreply = lacessManager.post(request, QJsonDocument(json).toJson());
    if(!vreply->isRunning())
        return false;
    //Espera solicitação voltar do servidor
    //CASO NUNCA VOLTE SÓ CRASHA RESOLVER
    while (!vreply->isFinished())
    {
        qApp->processEvents();
    }
    QByteArray response_data = nullptr;
    if(vreply->isReadable())
        response_data = vreply->readAll();
    else{
        qWarning() << vreply->errorString();
        return false;
    }
    if(vreply->isOpen())
    {
        vreply->close();
    }
    QJsonDocument jsond = QJsonDocument::fromJson(response_data);
    QString lreplyString = jsond.toJson();

    json.empty();
    query = lreplyString.toLatin1();
    //id do anime na lista
    QString lid = query.mid(query.lastIndexOf("id")+5);
    query = lid.left(lid.indexOf("\n"));
    lid = query;
    //Cria string com o pedido de delete
    QFile queryDelete(":/Anilist/qrc/Anilist/MutationDelete.txt");
    QTextStream queryDeleteStream(&queryDelete);
    if(!queryDelete.open(QIODevice::ReadOnly)){
        this->thread()->exit(0);
        return false;
    }
    query = queryDeleteStream.readAll();
    queryDelete.close();
    query.replace("variableID", lid);
    //Insere item no json
    json.insert("query", query);
    //Manda a solicitação de mudança
    vreply = lacessManager.post(request, QJsonDocument(json).toJson());
    while (!vreply->isFinished())
    {
        qApp->processEvents();
    }
    response_data = vreply->readAll();
    if(vreply->isOpen())
    {
        vreply->close();
    }
    jsond = QJsonDocument::fromJson(response_data);
    lreplyString = jsond.toJson();
    if(lreplyString.contains("error") || lreplyString.contains("errors"))
        return false;
    return true;
}
