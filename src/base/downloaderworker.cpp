#include "downloaderworker.h"

DownloaderWorker::DownloaderWorker(QObject *parent) : QObject(parent)
{
    vmanager = new QNetworkAccessManager(this);
    vreply = nullptr;
    vfileIsOpen = false;
    vlista = 0;
    vindexLista = 0;
    vfile = nullptr;
    m_busy = false;
}

bool DownloaderWorker::isBusy()
{
    return m_busy;
}

void DownloaderWorker::fsetLeitorListaAnimes(leitorlistaanimes *rleitorlistaanimes)
{
    cleitorlistaanimes = rleitorlistaanimes;
}

void DownloaderWorker::fsetConfBase(confBase *rconfbase)
{
    cconfBase = rconfbase;
}

void DownloaderWorker::fdownloadAvatarUsuario(int value)
{
    m_busy = true;
    QString lsaveFilePath = "Configurações/Temp/Imagens/avatar";
    lsaveFilePath.append(vURLAvatar.mid(vURLAvatar.lastIndexOf(QChar('.'))));

    if(QFile(lsaveFilePath).size() == 0)
        QFile(lsaveFilePath).remove();

    QNetworkRequest lrequest;
    lrequest.setUrl(QUrl(vURLAvatar));
    vreply = vmanager->get(lrequest);

    vfile = new QFile;
    vfile->setFileName(lsaveFilePath);
    vfile->open(QIODevice::WriteOnly);
    vfileIsOpen = true;

    connect(vmanager,SIGNAL(finished(QNetworkReply*)),this,SLOT(onFinished(QNetworkReply*)));
    connect(vreply,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
    connect(vreply,SIGNAL(finished()),this,SLOT(ffinishedAvatar()));
}

void DownloaderWorker::ffinishedAvatar()
{
    if(vfileIsOpen){
        if(vfile->isOpen())
            vfile->close();
    }
    vfileIsOpen = false;
    m_busy = false;
}

void DownloaderWorker::fsetURLAvatar(const QString &urlAvatar)
{
    vURLAvatar = urlAvatar;
}

void DownloaderWorker::fdownloadTorrent(const QString &fileURL, const QString &torrentName)
{

}

void DownloaderWorker::fdownloadXMLTorrentList(const QString &fileURL)
{

}

void DownloaderWorker::work(int value)
{
    m_busy = true;
    emit started();
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
            connect(vreply,SIGNAL(finished()),this,SLOT(ffinished()));
        }
        else{
            vfileIsOpen = false;
            ffinished();
        }
    }
    else{
        vlista++;
        m_busy = false;
        emit finished();
    }
}

void DownloaderWorker::ffinished()
{
//    if(vdownloadPequeno)
//        emit sidGrande(vlistaSelecionada[vindexLista]->vid);
//    else
//        emit sid(vlistaSelecionada[vindexLista]->vid);
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
    m_busy = false;
    emit finished();
}

void DownloaderWorker::workBig(int value)
{
    m_busy = true;
    emit started();
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
            connect(vreply,SIGNAL(finished()),this,SLOT(ffinishedBig()));
        }
        else{
            vfileIsOpen = false;
            ffinishedBig();
        }
    }
    else{
        vlista++;
        m_busy = false;
        emit finishedBig();
    }
}

void DownloaderWorker::ffinishedBig()
{
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
    m_busy = false;
    emit finishedBig();
}

void DownloaderWorker::onFinished(QNetworkReply *reply)
{
    switch(reply->error())
    {
        case QNetworkReply::NoError:
        {
//            qDebug("file is downloaded successfully.");
            if(vfile->isOpen())
            {
                vfile->close();
            }
        }break;
        default:{
//            qDebug() << reply->errorString();
            if(vfile->isOpen())
            {
                vfile->close();
            }
        }
    }
}

void DownloaderWorker::onReadyRead()
{
    if(vfile->isWritable()){
        QByteArray data = vreply->readAll();
        vfile->write(data);
        vfile->waitForBytesWritten(30000);
    }
    else{
        vfile->remove();
    }
}

void DownloaderWorker::fselecionaLista(QString rlista, QString rtipoLista)
{
    vindexLista = 0;
    vlista = 0;
    if(rtipoLista.compare("ANIME", Qt::CaseInsensitive) == 0){
        if(rlista.compare("CURRENT", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->retornaListaWatching();
        else if(rlista.compare("COMPLETED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->retornaListaCompleted();
        else if(rlista.compare("PAUSED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->retornaListaOnHold();
        else if(rlista.compare("DROPPED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->retornaListaDropped();
        else if(rlista.compare("PLANNING", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->retornaListaPlanToWatch();
    }
    else if(rtipoLista.compare("MANGA", Qt::CaseInsensitive) == 0){
        if(rlista.compare("CURRENT", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->retornaListaMangaReading();
        else if(rlista.compare("COMPLETED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->retornaListaMangaCompleted();
        else if(rlista.compare("PAUSED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->retornaListaMangaOnHold();
        else if(rlista.compare("DROPPED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->retornaListaMangaDropped();
        else if(rlista.compare("PLANNING", Qt::CaseInsensitive) == 0)
                vlistaSelecionada = cleitorlistaanimes->retornaListaMangaPlanToRead();
    }
    else if(rtipoLista.compare("NOVEL", Qt::CaseInsensitive) == 0){
        if(rlista.compare("CURRENT", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->retornaListaNovelReading();
        else if(rlista.compare("COMPLETED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->retornaListaNovelCompleted();
        else if(rlista.compare("PAUSED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->retornaListaNovelOnHold();
        else if(rlista.compare("DROPPED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->retornaListaNovelDropped();
        else if(rlista.compare("PLANNING", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->retornaListaNovelPlanToRead();
    }
    else if(rtipoLista.compare("SEASON", Qt::CaseInsensitive) == 0){
        if(rlista.contains("WINTER", Qt::CaseInsensitive)){
            rlista = rlista.remove("WINTER");
            vlistaSelecionada = cleitorlistaanimes->fleListaAnoSeason(rlista.toInt(), "winter");
        }
        else if(rlista.contains("SPRING", Qt::CaseInsensitive)){
            rlista = rlista.remove("SPRING");
            vlistaSelecionada = cleitorlistaanimes->fleListaAnoSeason(rlista.toInt(), "spring");
        }
        else if(rlista.contains("SUMMER", Qt::CaseInsensitive)){
            rlista = rlista.remove("SUMMER");
            vlistaSelecionada = cleitorlistaanimes->fleListaAnoSeason(rlista.toInt(), "summer");
        }
        else if(rlista.contains("FALL", Qt::CaseInsensitive)){
            rlista = rlista.remove("FALL");
            vlistaSelecionada = cleitorlistaanimes->fleListaAnoSeason(rlista.toInt(), "fall");
        }
        else
            vlistaSelecionada = cleitorlistaanimes->fleListaAno(rlista.toInt());
    }
}

