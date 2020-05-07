#include "client.h"

Client::Client(QObject *parent) : QObject(parent)
{

}


void Client::fselecionaClient(QVariant rnomeClient)
{
    if(rnomeClient.toString().compare("Anilist", Qt::CaseInsensitive) == 0){
        clientEscolhido = clients::ANILIST;
        clientAnilist = new anilist(this);
    }
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
            if(clientAnilist->fgetList())
                return true;
        break;
    }
    return false;
}

