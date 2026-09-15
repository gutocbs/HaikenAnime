#include "filedownloader.h"
#include <iostream>
filedownloader::filedownloader(QObject *parent) : QObject(parent)
{
    vmanager = new QNetworkAccessManager(this);
    vreply = nullptr;
    vfileIsOpen = false;
    vterminouLista = false;
    vdownloadAcontecendo = false;
    vdownloadAnoAcontecendo = false;
    vlista = 0;
    vindexLista = 0;
    vfile = nullptr;
    ano = 0;
}

filedownloader::~filedownloader(){
    vmanager->deleteLater();
    cconfBase->deleteLater();
    vfile->deleteLater();
    vreply->deleteLater();
}

void filedownloader::fsetLeitorListaAnimes(leitorlistaanimes *lleitorlistaanimes){
    cleitorlistaanimes = lleitorlistaanimes;
}

void filedownloader::fsetConfBase(confBase *rconfbase){
    cconfBase = rconfbase;
}

void filedownloader::onFinished(QNetworkReply * reply)
{
//    qDebug() << vfile->fileName() << reply->errorString();
    switch(reply->error())
    {
        case QNetworkReply::NoError:
        {
//            qDebug("file is downloaded successfully.");
            if(vfile->isOpen())
            {
                vfile->close();
            }
//            if(vfile->size() < 1000){
////                qDebug() << vfile->size();
//                vfile->remove();
//            }
        }break;
        default:{
        //Teste de remover a imagem se ocorrrer um erro no download
            if(vfile->isOpen())
            {
                vfile->close();
            }
        }
    }
}

void filedownloader::onReadyRead()
{
    if(vfile->isWritable()){
        QByteArray data = vreply->readAll();
        vfile->write(data);
        vfile->waitForBytesWritten(30000);
    }
    else{
//        qWarning() << vfile->errorString();// << "- Id:" << vlistaSelecionada[vindexLista]->vid << "- Medium image download failed";
        vfile->remove();
    }
}


void filedownloader::fdownloadAvatarUsuario(const QString &fileURL){
    QString lsaveFilePath = "Configurações/Temp/Imagens/avatar";
    lsaveFilePath.append(fileURL.mid(fileURL.lastIndexOf(QChar('.'))));

    if(QFile(lsaveFilePath).size() == 0)
        QFile(lsaveFilePath).remove();

    QNetworkRequest lrequest;
    lrequest.setUrl(QUrl(fileURL));
    vreply = vmanager->get(lrequest);

    vfile = new QFile;
    vfile->setFileName(lsaveFilePath);
    vfile->open(QIODevice::WriteOnly);
    vfileIsOpen = true;

    connect(vmanager,SIGNAL(finished(QNetworkReply*)),this,SLOT(onFinished(QNetworkReply*)));
    connect(vreply,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
    connect(vreply,SIGNAL(finished()),this,SLOT(fterminouAvatar()));
}

void filedownloader::fdownloadTorrent(const QString &fileURL, const QString &torrentName)
{
    QString lsaveFilePath = "Configurações/Temp/Torrents/";
    lsaveFilePath.append(torrentName);
    lsaveFilePath.append(".torrent");

    if(QFile(lsaveFilePath).size() == 0)
        QFile(lsaveFilePath).remove();

    if(QFile(lsaveFilePath).exists())
        emit storrent();

    QNetworkRequest lrequest;
    lrequest.setUrl(QUrl(fileURL));
    vreply = vmanager->get(lrequest);
    vfile = new QFile(this);
    vfile->setFileName(lsaveFilePath);
    vfile->open(QIODevice::WriteOnly);
    connect(vmanager,SIGNAL(finished(QNetworkReply*)),this,SLOT(onFinished(QNetworkReply*)));
    connect(vreply,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
    connect(vreply,SIGNAL(finished()),this,SIGNAL(storrent()));
}

void filedownloader::fdownloadXMLTorrentList(const QString &fileURL)
{
    QString lsaveFilePath = "Configurações/Temp/torrents.xml";

//    if(QFile(lsaveFilePath).size() == 0)
        QFile(lsaveFilePath).remove();

    QNetworkRequest lrequest;
    lrequest.setUrl(QUrl(fileURL));
    vreply = vmanager->get(lrequest);

    vfile = new QFile(this);
    vfile->setFileName(lsaveFilePath);
    vfile->open(QIODevice::WriteOnly);

    connect(vmanager,SIGNAL(finished(QNetworkReply*)),this,SLOT(onFinished(QNetworkReply*)));
    connect(vreply,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
    connect(vreply,SIGNAL(finished()),this,SIGNAL(sxml()));
}

void filedownloader::fsetDownload(bool rbaixarImagensPequenas)
{
    vdownloadPequeno = rbaixarImagensPequenas;
    if(vdownloadPequeno)
        fdownloadPequeno();
    else
        fdownloadMedio();
}

void filedownloader::fsetNext()
{
    if(vdownloadPequeno)
        emit sidGrande(vlistaSelecionada[vindexLista]->vid);
    else
        emit sid(vlistaSelecionada[vindexLista]->vid);
    if(vfileIsOpen){
        if(vfile->isOpen())
            vfile->close();
    }
    vfileIsOpen = false;
    if(vreply)
        vreply->close();
    vindexLista++;
    if(vindexLista >= vlistaSelecionada.size()){
        vindexLista = 0;
        vlista++;
    }
    fdownloadMedio();
}

void filedownloader::fsetNextBig()
{
    emit sidGrande(vlistaSelecionada[vindexLista]->vid);
    if(vfileIsOpen){
        if(vfile->isOpen())
            vfile->close();
    }
    vfileIsOpen = false;
    if(vreply)
        vreply->close();
    vindexLista++;
    if(vindexLista >= vlistaSelecionada.size()){
        vindexLista = 0;
        vlista++;
    }
    fdownloadGrande();
}

void filedownloader::fsetNextSmall()
{
    emit sid(vlistaSelecionada[vindexLista]->vid);
    if(vfileIsOpen){
        if(vfile->isOpen())
            vfile->close();
    }
    vfileIsOpen = false;
    if(vreply)
        vreply->close();
    vindexLista++;
    if(vindexLista >= vlistaSelecionada.size()){
        vindexLista = 0;
        vlista++;
    }
    fdownloadPequeno();
}

void filedownloader::fresetCounters()
{
    vindexLista = 0;
    vlista = 0;
    //    vindexListaPequeno = 0;
}

void filedownloader::fterminouAvatar()
{
    if(vfileIsOpen){
        if(vfile->isOpen())
            vfile->close();
    }
    vdownloadAcontecendo = false;
    emit savatar();
}

void filedownloader::fdownloadPorAno(int rano)
{
    if(!vdownloadAcontecendo){
        if(!vdownloadAnoAcontecendo)
            vlistaSelecionada = cleitorlistaanimes->fleListaAno(ano);
        vdownloadAnoAcontecendo = true;
        ano = rano;
        if(vindexLista >= vlistaSelecionada.size()){
            emit sterminouLista("ano");
            vindexLista = 0;
            vdownloadAnoAcontecendo = false;
            return;
        }
        if(!vlistaSelecionada.isEmpty()){
            vlink = vlistaSelecionada[vindexLista]->vLinkImagemMedia;
            if(vlink.contains("large"))
                vlink.replace("large", "medium");
            else if(vlink.contains("small"))
                vlink.replace("small", "medium");
            vsaveFilePath = cconfBase->vdiretorioImagensMedio;
            vsaveFilePath.append(vlistaSelecionada[vindexLista]->vid);
            vsaveFilePath.append(vlink.mid(vlink.lastIndexOf(QChar('.'))));
            if(QFile(vsaveFilePath).exists() && QFile(vsaveFilePath).size() == 0)
                QFile(vsaveFilePath).remove();

            if(!(QFileInfo::exists(vsaveFilePath) && QFileInfo(vsaveFilePath).isFile())){
                QNetworkRequest lrequest;
                lrequest.setUrl(QUrl(vlink));
                vreply = vmanager->get(lrequest);
                vfile = new QFile;
                vfile->setFileName(vsaveFilePath);
                vfile->open(QIODevice::WriteOnly);
                vfileIsOpen = true;
                connect(vmanager,SIGNAL(finished(QNetworkReply*)),this,SLOT(onFinished(QNetworkReply*)));
                connect(vreply,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
                connect(vreply,SIGNAL(finished()),this,SLOT(fsetNextAno()));
            }
            else{
                vfileIsOpen = false;
                fsetNextAno();
            }
        }
    }
}

void filedownloader::fsetNextAno()
{
    emit sid(vlistaSelecionada[vindexLista]->vid);
    if(vfileIsOpen){
        if(vfile->isOpen())
            vfile->close();
    }
    vfileIsOpen = false;
    if(vreply)
        vreply->close();
    vindexLista++;
    fdownloadPorAno(ano);
}

void filedownloader::fdownloadMedio(){
    if(vdownloadAnoAcontecendo)
        return;
    if(vlista == 0 && vlistaSelecionada != cleitorlistaanimes->retornaListaWatching()){
        vdownloadAcontecendo = true;
        vlistaSelecionada = cleitorlistaanimes->retornaListaWatching();
    }
    else if(vlista == 1 && vlistaSelecionada != cleitorlistaanimes->retornaListaCompleted())
        vlistaSelecionada = cleitorlistaanimes->retornaListaCompleted();
    else if(vlista == 2 && vlistaSelecionada != cleitorlistaanimes->retornaListaOnHold())
        vlistaSelecionada = cleitorlistaanimes->retornaListaOnHold();
    else if(vlista == 3 && vlistaSelecionada != cleitorlistaanimes->retornaListaDropped())
        vlistaSelecionada = cleitorlistaanimes->retornaListaDropped();
    else if(vlista == 4 && vlistaSelecionada != cleitorlistaanimes->retornaListaPlanToWatch())
        vlistaSelecionada = cleitorlistaanimes->retornaListaPlanToWatch();
    else if(vlista == 5 && vlistaSelecionada != cleitorlistaanimes->retornaListaMangaReading())
        vlistaSelecionada = cleitorlistaanimes->retornaListaMangaReading();
    else if(vlista == 6 && vlistaSelecionada != cleitorlistaanimes->retornaListaMangaCompleted())
        vlistaSelecionada = cleitorlistaanimes->retornaListaMangaCompleted();
    else if(vlista == 7 && vlistaSelecionada != cleitorlistaanimes->retornaListaMangaOnHold())
        vlistaSelecionada = cleitorlistaanimes->retornaListaMangaOnHold();
    else if(vlista == 8 && vlistaSelecionada != cleitorlistaanimes->retornaListaMangaDropped())
        vlistaSelecionada = cleitorlistaanimes->retornaListaMangaDropped();
    else if(vlista == 9 && vlistaSelecionada != cleitorlistaanimes->retornaListaMangaPlanToRead())
        vlistaSelecionada = cleitorlistaanimes->retornaListaMangaPlanToRead();
    else if(vlista == 10 && vlistaSelecionada != cleitorlistaanimes->retornaListaNovelReading())
        vlistaSelecionada = cleitorlistaanimes->retornaListaNovelReading();
    else if(vlista == 11 && vlistaSelecionada != cleitorlistaanimes->retornaListaNovelCompleted())
        vlistaSelecionada = cleitorlistaanimes->retornaListaNovelCompleted();
    else if(vlista == 12 && vlistaSelecionada != cleitorlistaanimes->retornaListaNovelOnHold())
        vlistaSelecionada = cleitorlistaanimes->retornaListaNovelOnHold();
    else if(vlista == 13 && vlistaSelecionada != cleitorlistaanimes->retornaListaNovelDropped())
        vlistaSelecionada = cleitorlistaanimes->retornaListaNovelDropped();
    else if(vlista == 14 && vlistaSelecionada != cleitorlistaanimes->retornaListaNovelPlanToRead())
        vlistaSelecionada = cleitorlistaanimes->retornaListaNovelPlanToRead();
    else if(vlista == 15){
        emit sterminouLista("medium");
        vindexLista = 0;
        vlista = 0;
        return;
    }
    if(!vlistaSelecionada.isEmpty()){
        vlink = vlistaSelecionada[vindexLista]->vLinkImagemMedia;
        if(vlink.contains("large"))
            vlink.replace("large", "medium");
        else if(vlink.contains("small"))
            vlink.replace("small", "medium");
        vsaveFilePath = cconfBase->vdiretorioImagensMedio;
        vsaveFilePath.append(vlistaSelecionada[vindexLista]->vid);
        vsaveFilePath.append(vlink.mid(vlink.lastIndexOf(QChar('.'))));
        if(QFile(vsaveFilePath).exists() && QFile(vsaveFilePath).size() == 0)
            QFile(vsaveFilePath).remove();

        if(!(QFileInfo::exists(vsaveFilePath) && QFileInfo(vsaveFilePath).isFile())){
            QNetworkRequest lrequest;
            lrequest.setUrl(QUrl(vlink));
            vreply = vmanager->get(lrequest);

            vfile = new QFile;
            vfile->setFileName(vsaveFilePath);
            vfile->open(QIODevice::WriteOnly);
            vfileIsOpen = true;
            connect(vmanager,SIGNAL(finished(QNetworkReply*)),this,SLOT(onFinished(QNetworkReply*)));
            connect(vreply,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
            connect(vreply,SIGNAL(finished()),this,SLOT(fsetNext()));
        }
        else{
            vfileIsOpen = false;
            fsetNext();
        }
    }
    else{
        vlista++;
        fdownloadMedio();
    }
}

void filedownloader::fdownloadGrande(){
    if(vdownloadAnoAcontecendo)
        return;
    if(vlista == 0 && vlistaSelecionada != cleitorlistaanimes->retornaListaWatching())
        vlistaSelecionada = cleitorlistaanimes->retornaListaWatching();
    else if(vlista == 1 && vlistaSelecionada != cleitorlistaanimes->retornaListaCompleted())
        vlistaSelecionada = cleitorlistaanimes->retornaListaCompleted();
    else if(vlista == 2 && vlistaSelecionada != cleitorlistaanimes->retornaListaOnHold())
        vlistaSelecionada = cleitorlistaanimes->retornaListaOnHold();
    else if(vlista == 3 && vlistaSelecionada != cleitorlistaanimes->retornaListaDropped())
        vlistaSelecionada = cleitorlistaanimes->retornaListaDropped();
    else if(vlista == 4 && vlistaSelecionada != cleitorlistaanimes->retornaListaPlanToWatch())
        vlistaSelecionada = cleitorlistaanimes->retornaListaPlanToWatch();
    else if(vlista == 5 && vlistaSelecionada != cleitorlistaanimes->retornaListaMangaReading())
        vlistaSelecionada = cleitorlistaanimes->retornaListaMangaReading();
    else if(vlista == 6 && vlistaSelecionada != cleitorlistaanimes->retornaListaMangaCompleted())
        vlistaSelecionada = cleitorlistaanimes->retornaListaMangaCompleted();
    else if(vlista == 7 && vlistaSelecionada != cleitorlistaanimes->retornaListaMangaOnHold())
        vlistaSelecionada = cleitorlistaanimes->retornaListaMangaOnHold();
    else if(vlista == 8 && vlistaSelecionada != cleitorlistaanimes->retornaListaMangaDropped())
        vlistaSelecionada = cleitorlistaanimes->retornaListaMangaDropped();
    else if(vlista == 9 && vlistaSelecionada != cleitorlistaanimes->retornaListaMangaPlanToRead())
        vlistaSelecionada = cleitorlistaanimes->retornaListaMangaPlanToRead();
    else if(vlista == 10 && vlistaSelecionada != cleitorlistaanimes->retornaListaNovelReading())
        vlistaSelecionada = cleitorlistaanimes->retornaListaNovelReading();
    else if(vlista == 11 && vlistaSelecionada != cleitorlistaanimes->retornaListaNovelCompleted())
        vlistaSelecionada = cleitorlistaanimes->retornaListaNovelCompleted();
    else if(vlista == 12 && vlistaSelecionada != cleitorlistaanimes->retornaListaNovelOnHold())
        vlistaSelecionada = cleitorlistaanimes->retornaListaNovelOnHold();
    else if(vlista == 13 && vlistaSelecionada != cleitorlistaanimes->retornaListaNovelDropped())
        vlistaSelecionada = cleitorlistaanimes->retornaListaNovelDropped();
    else if(vlista == 14 && vlistaSelecionada != cleitorlistaanimes->retornaListaNovelPlanToRead())
        vlistaSelecionada = cleitorlistaanimes->retornaListaNovelPlanToRead();
    else if(vlista == 15){
        emit sterminouLista("big");
        vindexLista = 0;
        vlista = 0;
        return;
    }
    if(!vlistaSelecionada.isEmpty()){
        vlink = vlistaSelecionada[vindexLista]->vLinkImagemMedia;
        if(vlink.contains("medium"))
            vlink.replace("medium", "large");
        else if(vlink.contains("small"))
            vlink.replace("small", "large");
        vsaveFilePath = cconfBase->vdiretorioImagensGrandes;
        vsaveFilePath.append(vlistaSelecionada[vindexLista]->vid);
        vsaveFilePath.append(vlink.mid(vlink.lastIndexOf(QChar('.'))));

        if(QFile(vsaveFilePath).exists() && QFile(vsaveFilePath).size() == 0)
            QFile(vsaveFilePath).remove();
    //    qDebug() << vsaveFilePath << vlistaSelecionada[vindexLista]->vid << vlistaSelecionada[vindexLista]->vLinkImagemMedia.mid(
    //                    vlistaSelecionada[vindexLista]->vLinkImagemMedia.lastIndexOf(QChar('.')));
        if(!(QFileInfo::exists(vsaveFilePath) && QFileInfo(vsaveFilePath).isFile())){
    //        qDebug() << vlistaSelecionada[vindexLista]->vid;
            QNetworkRequest lrequest;
            lrequest.setUrl(QUrl(vlink));
            vreply = vmanager->get(lrequest);

            vfile = new QFile;
            vfile->setFileName(vsaveFilePath);
            vfile->open(QIODevice::WriteOnly);
            vfileIsOpen = true;
            connect(vmanager,SIGNAL(finished(QNetworkReply*)),this,SLOT(onFinished(QNetworkReply*)));
            connect(vreply,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
            connect(vreply,SIGNAL(finished()),this,SLOT(fsetNextBig()));
        }
        else{
            vfileIsOpen = false;
            fsetNextBig();
        }
    }
    else{
        vlista++;
        fdownloadGrande();
    }
}

void filedownloader::fdownloadPequeno(){
    if(vdownloadAnoAcontecendo)
        return;
    if(vlista == 0 && vlistaSelecionada != cleitorlistaanimes->retornaListaWatching())
        vlistaSelecionada = cleitorlistaanimes->retornaListaWatching();
    else if(vlista == 1 && vlistaSelecionada != cleitorlistaanimes->retornaListaCompleted())
        vlistaSelecionada = cleitorlistaanimes->retornaListaCompleted();
    else if(vlista == 2 && vlistaSelecionada != cleitorlistaanimes->retornaListaOnHold())
        vlistaSelecionada = cleitorlistaanimes->retornaListaOnHold();
    else if(vlista == 3 && vlistaSelecionada != cleitorlistaanimes->retornaListaDropped())
        vlistaSelecionada = cleitorlistaanimes->retornaListaDropped();
    else if(vlista == 4 && vlistaSelecionada != cleitorlistaanimes->retornaListaPlanToWatch())
        vlistaSelecionada = cleitorlistaanimes->retornaListaPlanToWatch();
    else if(vlista == 5 && vlistaSelecionada != cleitorlistaanimes->retornaListaMangaReading())
        vlistaSelecionada = cleitorlistaanimes->retornaListaMangaReading();
    else if(vlista == 6 && vlistaSelecionada != cleitorlistaanimes->retornaListaMangaCompleted())
        vlistaSelecionada = cleitorlistaanimes->retornaListaMangaCompleted();
    else if(vlista == 7 && vlistaSelecionada != cleitorlistaanimes->retornaListaMangaOnHold())
        vlistaSelecionada = cleitorlistaanimes->retornaListaMangaOnHold();
    else if(vlista == 8 && vlistaSelecionada != cleitorlistaanimes->retornaListaMangaDropped())
        vlistaSelecionada = cleitorlistaanimes->retornaListaMangaDropped();
    else if(vlista == 9 && vlistaSelecionada != cleitorlistaanimes->retornaListaMangaPlanToRead())
        vlistaSelecionada = cleitorlistaanimes->retornaListaMangaPlanToRead();
    else if(vlista == 10 && vlistaSelecionada != cleitorlistaanimes->retornaListaNovelReading())
        vlistaSelecionada = cleitorlistaanimes->retornaListaNovelReading();
    else if(vlista == 11 && vlistaSelecionada != cleitorlistaanimes->retornaListaNovelCompleted())
        vlistaSelecionada = cleitorlistaanimes->retornaListaNovelCompleted();
    else if(vlista == 12 && vlistaSelecionada != cleitorlistaanimes->retornaListaNovelOnHold())
        vlistaSelecionada = cleitorlistaanimes->retornaListaNovelOnHold();
    else if(vlista == 13 && vlistaSelecionada != cleitorlistaanimes->retornaListaNovelDropped())
        vlistaSelecionada = cleitorlistaanimes->retornaListaNovelDropped();
    else if(vlista == 14 && vlistaSelecionada != cleitorlistaanimes->retornaListaNovelPlanToRead())
        vlistaSelecionada = cleitorlistaanimes->retornaListaNovelPlanToRead();
    else if(vlista == 15){
        emit sterminouLista("small");
        vindexLista = 0;
        vlista = 0;
        return;
    }
    if(!vlistaSelecionada.isEmpty()){
        vlink = vlistaSelecionada[vindexLista]->vLinkImagemMedia;
        if(vlink.contains("large"))
            vlink.replace("large", "small");
        else if(vlink.contains("medium"))
            vlink.replace("medium", "small");
        vsaveFilePath = cconfBase->vdiretorioImagensPequenas;
        vsaveFilePath.append(vlistaSelecionada[vindexLista]->vid);
        vsaveFilePath.append(vlink.mid(vlink.lastIndexOf(QChar('.'))));
        if(QFile(vsaveFilePath).exists() && QFile(vsaveFilePath).size() == 0)
            QFile(vsaveFilePath).remove();

        if(!(QFileInfo::exists(vsaveFilePath) && QFileInfo(vsaveFilePath).isFile())){
            QNetworkRequest lrequest;
            lrequest.setUrl(QUrl(vlink));
            vreply = vmanager->get(lrequest);

            vfile = new QFile;
            vfile->setFileName(vsaveFilePath);
            vfile->open(QIODevice::WriteOnly);
            vfileIsOpen = true;
            connect(vmanager,SIGNAL(finished(QNetworkReply*)),this,SLOT(onFinished(QNetworkReply*)));
            connect(vreply,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
            connect(vreply,SIGNAL(finished()),this,SLOT(fsetNextSmall()));
        }
        else{
            vfileIsOpen = false;
            fsetNextSmall();
        }
    }
    else{
        vlista++;
        fdownloadPequeno();
    }
}
