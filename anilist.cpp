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
    QString totalPages = "query ($id: Int) {   Page (page: 1 perPage: 50) {     pageInfo {       total        currentPage        lastPage        hasNextPage        perPage     }     mediaList(userName: " + vusername + ", id: $id, sort: MEDIA_ID) {       user{         id          name         avatar{           large         }       }     }   } }";
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
            QString query = "query ($id: Int, $perPage: Int) { Page (page:" + QString::number(i) + ", perPage: $perPage) {  		mediaList(userName: " + vusername + ", id: $id, sort: MEDIA_ID) {  			status  			score  			progress  			media{ 				format 				averageScore 				id 				title{ 					romaji 					english 				} 				synonyms 				description 				status 				coverImage{ 					large 				} 				season 				startDate { 					year 					month 				} 				chapters 				volumes 				episodes 				nextAiringEpisode{ 					 					airingAt 					episode 				} 				siteUrl 				streamingEpisodes{ 					site 					url 				}  			} 		  		} 	  	}  }";
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
    this->thread()->exit(0);
    vreply->deleteLater();
    return true;
}

bool anilist::fgetListasAnoSeason()
{
    for(int i = 0; i <  QDate::currentDate().year()-1999; i++){
        fgetListaAno(QString::number(2000+i));
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
    QString totalPages = "query ($id: Int) {   Page (page: 1 perPage: 50) {     pageInfo {       total        currentPage        lastPage        hasNextPage        perPage     }     media(id: $id, seasonYear: " + rano + ", isAdult: false, sort: ID) {       status     }   } }";
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
            QString query = "query ($id: Int, $perPage: Int) {  	 	Page (page:" + QString::number(i) + ", perPage: $perPage) {  		 		 		media(id: $id, seasonYear: " + rano + ", isAdult: false, sort: ID) {  			 			 			status    			 			 				 				 			format 				 				 			averageScore 				 				 			id 				 				 			title{ 					 					 				romaji 					 					 				english 				 				 			} 				 				 			synonyms 				 				 			description 				 				 			status 				 				 			coverImage{ 					 					 				large 				 				 			} 				 				 			season 				 				 			startDate { 					 					 				year 					 					 				month 				 				 			} 				 				 			chapters 				 				 			volumes 				 				 			episodes 				 				 			nextAiringEpisode{ 					 					 					 				airingAt 					 					 				episode 				 				 			} 				 				 			siteUrl 				 				 			streamingEpisodes{ 					 					 				site 					 					 				url 				 				 			}  			 			 		  		 		 		} 	  	 	 	}    }";
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
    //Cria string com a lista antiga
    QString llistaAtual = "mutation{     SaveMediaListEntry (mediaId: " + QString::number(rid) + ", status: " + rNovaLista + ") {         id         status     } }";
    //Insere item no json
    json.insert("query", llistaAtual);
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
    ///Preciso por o token em um arquvio de configuração criptografado
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
    //Cria string com a lista antiga
    QString llistaAtual = "mutation{     SaveMediaListEntry (mediaId: " + QString::number(rid) + ", score: " + QString::number(rnovaNota) + ") {         id         status     } }";
    //Insere item no json
    json.insert("query", llistaAtual);
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
    ///Preciso por o token em um arquvio de configuração criptografado
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
    //Cria string com a lista antiga
    QString llistaAtual = "mutation{     SaveMediaListEntry (mediaId: " + QString::number(rid) + ", progress: " + QString::number(rnovoProgresso) + ") {         id         status     } }";
    //Insere item no json
    json.insert("query", llistaAtual);
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

void anilist::frecebeAutorizacao(const QString &ruser, QString rauthcode)
{
    vusername = "\"" + ruser + "\"";
    vtoken = std::move(rauthcode);
}

bool anilist::fexcluiAnime(int rid){
    ///Preciso por o token em um arquvio de configuração criptografado
    QByteArray auth = "Bearer ";
    auth.append(vtoken);

    QNetworkRequest lrequest(graphqlUrl);
    lrequest.setRawHeader(QByteArray("Authorization"), auth);
    lrequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    lrequest.setRawHeader(QByteArray("Accept"), "application/json; charset=utf-8");
    QJsonObject json;
    QNetworkAccessManager lacessManager;

    //Cria string com o pedido de delete
    QString llistaAtual =  "query{ MediaList(mediaId:" + QString::number(rid) + ", userName: " + vusername + "){ id }}";
    //Insere item no json
    json.insert("query", llistaAtual);
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
    llistaAtual = lreplyString.toLatin1();
    //id do anime na lista
    QString lid = llistaAtual.mid(llistaAtual.lastIndexOf("id")+5);
    llistaAtual = lid.left(lid.indexOf("\n"));
    lid = llistaAtual;
    llistaAtual = "mutation{     DeleteMediaListEntry (id: " + lid + ") {      deleted     } }";
    //Insere item no json
    json.insert("query", llistaAtual);
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
