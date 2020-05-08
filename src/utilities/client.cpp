#include "client.h"

Client::Client(QObject *parent) : QObject(parent)
{
}

Client::~Client()
{
    tthreadClient.requestInterruption();
    tthreadClient.wait();
    switch (clientEscolhido) {
        case ANILIST:
            clientAnilist->deleteLater();
        break;
    }
}

void Client::fconnections(){
    switch (clientEscolhido) {
        case ANILIST:
            //Ao terminar de baixar a lista, manda um aviso que devo mandar pra main class
            connect(clientAnilist, &anilist::sterminouDownload, this, &Client::fdownloadSignal);
        break;
    }
}

void Client::fdownloadSignal(bool downloadSignal){
    emit sdownloadCompleted(downloadSignal);
}

void Client::fselecionaClient(QVariant rnomeClient)
{
    if(rnomeClient.toString().compare("Anilist", Qt::CaseInsensitive) == 0){
        clientEscolhido = clients::ANILIST;
        clientAnilist = new anilist();
    }
    fconnections();
}

void Client::frecebeAutorizacao(const QString &ruser, QString rauthcode)
{
    switch (clientEscolhido) {
        case ANILIST:
            clientAnilist->frecebeAutorizacao(ruser, rauthcode);
        break;
    }
}
void Client::fbaixaListas()
{
    if(!tthreadClient.isRunning()){
        fpassaThread();
        tthreadClient.start();
    }
}

QString Client::fgetAvatar()
{
    switch (clientEscolhido) {
        case ANILIST:
            return clientAnilist->fretornaAvatar();
        break;
    }
}

void Client::fpassaThread()
{
    switch (clientEscolhido) {
        case ANILIST:
            clientAnilist->fbaixaListaThread(tthreadClient);
            clientAnilist->moveToThread(&tthreadClient);
        break;
    }
}

bool Client::fmudaLista(int ridAnime, const QString &rNovaLista)
{
    switch (clientEscolhido) {
        case ANILIST:
            if(clientAnilist->fmudaLista(ridAnime,rNovaLista))
                    return true;
        break;
    }
    return false;
}

bool Client::fmudaNota(int ridAnime, int rnovaNota)
{
    switch (clientEscolhido) {
        case ANILIST:
            if(clientAnilist->fmudaNota(ridAnime,rnovaNota))
                return true;
        break;
    }
    return false;
}

bool Client::fmudaProgresso(int ridAnime, int rprogresso)
{
    switch (clientEscolhido) {
        case ANILIST:
            if(clientAnilist->fmudaProgresso(ridAnime, rprogresso))
                    return true;
        break;
    }
    return false;
}

bool Client::fexcluiAnime(int ridAnime)
{
    switch (clientEscolhido) {
        case ANILIST:
            if(clientAnilist->fexcluiAnime(ridAnime))
                    return true;
        break;
    }
    return false;
}

bool Client::fgetListaPorAno()
{
    switch (clientEscolhido) {
        case ANILIST:
            if(clientAnilist->fgetListasAnoSeason())
                return true;
        break;
    }
    return false;
}

