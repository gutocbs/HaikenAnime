#include "client.h"

Client::Client(QObject *parent) : QObject(parent)
{
}

Client::~Client()
{
    tthreadClient.requestInterruption();
    tthreadClient.wait();
    switch (clientEscolhido) {
        case abaConfig::ANILIST:
            clientAnilist->deleteLater();
        break;
    }
}

void Client::fconnections(){
    switch (clientEscolhido) {
        case abaConfig::ANILIST:
            //Ao terminar de baixar a lista, manda um aviso que devo mandar pra main class
            connect(clientAnilist, &anilist::downloadFinished, this, &Client::fdownloadSignal);
        break;
    }
}

void Client::fdownloadSignal(bool downloadSignal){
    emit sdownloadCompleted(downloadSignal);
}

void Client::fselecionaClient(QVariant rnomeClient)
{
    if(rnomeClient.toString().compare("Anilist", Qt::CaseInsensitive) == 0){
        clientEscolhido = abaConfig::clients::ANILIST;
        clientAnilist = new anilist();
    }
    fconnections();
}

void Client::frecebeAutorizacao(const QString &ruser, QVariant rauthcode)
{
    switch (clientEscolhido) {
        case abaConfig::ANILIST:
            clientAnilist->setAuthCode(ruser, rauthcode);
        break;
    }
}
void Client::downloadMediaList()
{
    if(!tthreadClient.isRunning()){
        fpassaThread();
        tthreadClient.start();
    }
}

QString Client::fgetAvatar()
{
    switch (clientEscolhido) {
        case abaConfig::ANILIST:
            return clientAnilist->getAvatar();
        break;
    }
}

void Client::fpassaThread()
{
    switch (clientEscolhido) {
        case abaConfig::ANILIST:
            clientAnilist->setThread(tthreadClient);
            clientAnilist->moveToThread(&tthreadClient);
        break;
    }
}

bool Client::fmudaLista(int ridAnime, const QString &rNovaLista)
{
    switch (clientEscolhido) {
        case abaConfig::ANILIST:
            if(clientAnilist->updateList(ridAnime,Enums::QStringToMediaList(rNovaLista)))
                return true;
        break;
    }
    return false;
}

bool Client::fmudaNota(int ridAnime, int rnovaNota)
{
    switch (clientEscolhido) {
        case abaConfig::ANILIST:
            if(clientAnilist->updateScore(ridAnime,rnovaNota))
                return true;
        break;
    }
    return false;
}

bool Client::fmudaProgresso(int ridAnime, int rprogresso)
{
    switch (clientEscolhido) {
        case abaConfig::ANILIST:
            if(clientAnilist->updateProgress(ridAnime, rprogresso))
                    return true;
        break;
    }
    return false;
}

bool Client::fexcluiAnime(int ridAnime)
{
    switch (clientEscolhido) {
        case abaConfig::ANILIST:
            if(clientAnilist->deleteMediaFromList(ridAnime))
                    return true;
        break;
    }
    return false;
}

bool Client::fgetListaPorAno()
{
    switch (clientEscolhido) {
        case abaConfig::ANILIST:
            if(clientAnilist->getYearlyLists())
                return true;
        break;
    }
    return false;
}

