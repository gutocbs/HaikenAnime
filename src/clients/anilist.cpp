#include "anilist.h"
#include <utility>

const QUrl graphqlUrl("https://graphql.anilist.co");

anilist::anilist(QObject *parent) : QObject(parent)
{
//    vusername = "\"gutocbs\"";
//    vtoken = "";
}

anilist::~anilist(){
//    vreply->deleteLater();
}

bool anilist::fgetList(){
    //Cria o pedido em javascript
    QNetworkRequest lrequest(graphqlUrl);
    lrequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    lrequest.setRawHeader(QByteArray("Accept"), "application/json; charset=utf-8");
    QJsonObject json;
    QNetworkAccessManager lacessManager;

    //Query que irá solicitar o avatar e o número de páginas que temos que pegar
    QFile avatarTotalPages(":/Anilist/qrc/Anilist/AvatarTotalPages.txt");
    QTextStream textStream(&avatarTotalPages);
    if(!avatarTotalPages.open(QIODevice::ReadOnly)){
        this->thread()->exit(0);
        return false;
    }
    QString totalPages = textStream.readAll();
    avatarTotalPages.close();
    totalPages.replace("variableUsername", vusername);
    json.insert("query", totalPages);

    //Checa se a thread está sendo interrompida, ou seja, se o programa está sendo fechado durante a execução da função
    //Isso vai ocorrer em diversos pontos da thread por conta dos loops
    if(this->thread()->isInterruptionRequested()){
        this->thread()->exit(0);
        return false;
    }

    //Post faz o pedido ao servidor lrequest, usando os argumentos em Json
//    QPointer<QNetworkReply> vreply = lacessManager.post(lrequest, QJsonDocument(json).toJson());
    QNetworkReply* vreply = lacessManager.post(lrequest, QJsonDocument(json).toJson());
    //Espera uma resposta
    while (!vreply->isFinished())
    {
        if(this->thread()->isInterruptionRequested()){
            this->thread()->exit(0);
            return false;
        }
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
    QString lastpage = jsond.toJson();
    //Verificamos se é uma mensagem de erro
    if(lastpage.contains("errors")){
        this->thread()->exit(0);
        emit sterminouDownload(false);
        vreply->deleteLater();
        return false;
    }
    lastpage = lastpage.toLatin1();
    //Pega avatar
    QString llastpage = lastpage.mid(lastpage.lastIndexOf("avatar"));
    vavatar = llastpage.left(llastpage.indexOf("\"\n"));
    //Pega total de páginas
    llastpage = lastpage.mid(lastpage.lastIndexOf("lastPage")+11);
    lastpage = llastpage.left(llastpage.indexOf(",\n"));

    if(this->thread()->isInterruptionRequested()){
        this->thread()->exit(0);
        vreply->deleteLater();
        return false;
    }

    QFile t("Configurações/Temp/animeListTemp.txt");
    if(t.open(QIODevice::WriteOnly)){
        for(int i = 1; i < lastpage.toInt()+1; i++){
            if(this->thread()->isInterruptionRequested()){
                this->thread()->exit(0);
                vreply->deleteLater();
                return false;
            }
            QFile animeList(":/Anilist/qrc/Anilist/AnimeInfo.txt");
            QTextStream textStream(&animeList);
            if(!animeList.open(QIODevice::ReadOnly)){
                this->thread()->exit(0);
                return false;
            }
            QString query = textStream.readAll();
            animeList.close();
            query.replace("variablePage", QString::number(i));
            query.replace("variableUsername", vusername);
            json.insert("query", query.trimmed());
            vreply = lacessManager.post(lrequest, QJsonDocument(json).toJson());
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
        emit sterminouDownload(false);
        this->thread()->exit(0);
        vreply->deleteLater();
        return false;
    }
    if(QFile::exists("Configurações/Temp/animeList.txt")){
        if(QFile::remove("Configurações/Temp/animeList.txt"))
            t.rename("Configurações/Temp/animeList.txt");
    }
    else
        t.rename("Configurações/Temp/animeList.txt");
    emit sterminouDownload(true);
    fgetListasAnoSeason();
    this->thread()->exit(0);
    vreply->deleteLater();
    return true;
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
    if(QFile::exists("Configurações/Temp/Lists/animeList"+rano+".txt") && rano != QString::number(QDate::currentDate().year())){
        return true;
    }
    //Cria o pedido em javascript
    QNetworkRequest lrequest(graphqlUrl);
    lrequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    lrequest.setRawHeader(QByteArray("Accept"), "application/json; charset=utf-8");
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

    //Post faz o pedido ao servidor lrequest, usando os argumentos em Json
//    QPointer<QNetworkReply> vreply = lacessManager.post(lrequest, QJsonDocument(json).toJson());
    QNetworkReply* vreply = lacessManager.post(lrequest, QJsonDocument(json).toJson());
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
    QString lastpage = jsond.toJson();
    //Verificamos se é uma mensagem de erro
    if(lastpage.contains("errors")){
        this->thread()->exit(0);
        vreply->deleteLater();
        return false;
    }
    lastpage = lastpage.toLatin1();
    //Pega avatar
    QString llastpage = lastpage.mid(lastpage.lastIndexOf("avatar"));
    vavatar = llastpage.left(llastpage.indexOf("\"\n"));
    //Pega total de páginas
    llastpage = lastpage.mid(lastpage.lastIndexOf("lastPage")+11);
    lastpage = llastpage.left(llastpage.indexOf(",\n"));

    QFile t("Configurações/Temp/Lists/animeList"+rano+"Temp.txt");
    if(t.open(QIODevice::WriteOnly)){
        for(int i = 1; i < lastpage.toInt()+1; i++){
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
            vreply = lacessManager.post(lrequest, QJsonDocument(json).toJson());
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
    QByteArray auth = "Bearer ";
    auth.append(vtoken);

    QNetworkRequest lrequest(graphqlUrl);
    lrequest.setRawHeader(QByteArray("Authorization"), auth);
    lrequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    lrequest.setRawHeader(QByteArray("Accept"), "application/json; charset=utf-8");
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
    QPointer<QNetworkReply> vreply = lacessManager.post(lrequest, QJsonDocument(json).toJson());
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
    QByteArray auth = "Bearer ";
    auth.append(vtoken);

    //Como pegar o nome ou id de usuário pelo token?
    //Posso pegar igual fazia antes,/ sem o token?
    QNetworkRequest lrequest(graphqlUrl);
    lrequest.setRawHeader(QByteArray("Authorization"), auth);
    lrequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    lrequest.setRawHeader(QByteArray("Accept"), "application/json; charset=utf-8");
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
    QPointer<QNetworkReply> vreply = lacessManager.post(lrequest, QJsonDocument(json).toJson());


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
    QByteArray auth = "Bearer ";
    auth.append(vtoken);

    //Como pegar o nome ou id de usuário pelo token?
    //Posso pegar igual fazia antes,/ sem o token?
    QNetworkRequest lrequest(graphqlUrl);
    lrequest.setRawHeader(QByteArray("Authorization"), auth);
    lrequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    lrequest.setRawHeader(QByteArray("Accept"), "application/json; charset=utf-8");
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
    QPointer<QNetworkReply> vreply = lacessManager.post(lrequest, QJsonDocument(json).toJson());

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
    QByteArray auth = "Bearer ";
    auth.append(vtoken);

    QNetworkRequest lrequest(graphqlUrl);
    lrequest.setRawHeader(QByteArray("Authorization"), auth);
    lrequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    lrequest.setRawHeader(QByteArray("Accept"), "application/json; charset=utf-8");
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
    QPointer<QNetworkReply> vreply = lacessManager.post(lrequest, QJsonDocument(json).toJson());
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
    QFile queryDelete(":/Anilist/qrc/Anilist/QueryDelete.txt");
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
    vreply = lacessManager.post(lrequest, QJsonDocument(json).toJson());
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
