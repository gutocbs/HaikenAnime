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
    cconfBase = new confBase;
//    cleitorlistaanimes = new leitorlistaanimes();
    //    cleitorlistaanimes->instance();
}

DownloaderWorker::~DownloaderWorker()
{
    vreply->deleteLater();
}

bool DownloaderWorker::isBusy()
{
    return m_busy;
}

void DownloaderWorker::fdownloadAvatarUsuario(int value)
{
    Q_UNUSED(value)
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
    QFile *tempFile = vfile;
    vfile = nullptr;
    tempFile->deleteLater();

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

void DownloaderWorker::fdownloadXMLTorrentList()
{
    QString lsaveFilePath = "Configurações/Temp/torrents.xml";

//    if(QFile(lsaveFilePath).size() == 0)
        QFile(lsaveFilePath).remove();
    QPointer<abaConfig> cabaConfig;
    QNetworkRequest lrequest;
    lrequest.setUrl(QUrl(cabaConfig->instance()->fgetGeneralFeed()));
    vreply = vmanager->get(lrequest);

    vfile = new QFile(this);
    vfile->setFileName(lsaveFilePath);
    vfile->open(QIODevice::WriteOnly);

    connect(vmanager,SIGNAL(finished(QNetworkReply*)),this,SLOT(onFinished(QNetworkReply*)));
    connect(vreply,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
    connect(vreply,SIGNAL(finished()),this,SLOT(ffinishedGeneralXML()));
}

void DownloaderWorker::fdownloadSpecificXMLTorrentList(QString searchName)
{
    QString lsaveFilePath = "Configurações/Temp/torrents.xml";

//    if(QFile(lsaveFilePath).size() == 0)
        QFile(lsaveFilePath).remove();
    QPointer<abaConfig> cabaConfig;
    QNetworkRequest lrequest;
    QString searchULR = cabaConfig->instance()->fgetSpecificFeed();
    searchULR.replace("%title%", searchName);
    lrequest.setUrl(QUrl(searchULR));
    vreply = vmanager->get(lrequest);

    vfile = new QFile(this);
    vfile->setFileName(lsaveFilePath);
    vfile->open(QIODevice::WriteOnly);

    connect(vmanager,SIGNAL(finished(QNetworkReply*)),this,SLOT(onFinished(QNetworkReply*)));
    connect(vreply,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
    connect(vreply,SIGNAL(finished()),this,SLOT(ffinishedSpecificXML()));
}

void DownloaderWorker::fdownloadAnimeTorrent(QString linkTorrent, QString nomeTorrent, int posicaoLista)
{
    m_busy = true;
    emit started();

    vsaveFilePath = "Configurações/Temp/Torrents/";
    vsaveFilePath.append(nomeTorrent);
    vsaveFilePath.append(".torrent");
    vlink = linkTorrent;
    qDebug() << vsaveFilePath;
//    if(QFile(vsaveFilePath).exists() && QFile(vsaveFilePath).size() == 0)
    if(QFile::exists(vsaveFilePath)){
        QFile::remove(vsaveFilePath);
        qDebug() << "removido";
    }

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
        connect(vreply,SIGNAL(finished()),this,SLOT(ffinishedAnimeTorrent()));
    }
    else
        ffinishedAnimeTorrent();
}

void DownloaderWorker::ffinishedGeneralXML()
{
    if(vfileIsOpen){
        if(vfile->isOpen())
            vfile->close();
    }
    QFile *tempFile = vfile;
    vfile = nullptr;
    tempFile->deleteLater();

    vfileIsOpen = false;
    m_busy = false;
    emit finishedXML();
}

void DownloaderWorker::ffinishedSpecificXML()
{
    if(vfileIsOpen){
        if(vfile->isOpen())
            vfile->close();
    }
    QFile *tempFile = vfile;
    vfile = nullptr;
    tempFile->deleteLater();

    vfileIsOpen = false;
    m_busy = false;
    emit finishedSpecificXML();
}

void DownloaderWorker::ffinishedAnimeTorrent()
{
    if(vfileIsOpen){
        if(vfile->isOpen())
            vfile->close();
    }
    QFile *tempFile = vfile;
    vfile = nullptr;
    tempFile->deleteLater();

    vfileIsOpen = false;
    if(vreply)
        vreply->close();

    m_busy = false;
    emit finishedAnimeTorrent();
}

void DownloaderWorker::work(int value)
{
    Q_UNUSED(value)
    m_busy = true;
    emit started();
    if(!vlistaSelecionada.isEmpty()){
        vlink = vlistaSelecionada[vindexLista]->vLinkImagemMedia;
        if(vlink.contains("large"))
            vlink.replace("large", "medium");
        else if(vlink.contains("small"))
            vlink.replace("small", "medium");
        vsaveFilePath = cconfBase->instance()->vdiretorioImagensMedio;
        vsaveFilePath.append(vlistaSelecionada[vindexLista]->vid);
        vsaveFilePath.append(vlink.mid(vlink.lastIndexOf(QChar('.'))));
        if(QFile(vsaveFilePath).exists() && QFile(vsaveFilePath).size() == 0)
            QFile(vsaveFilePath).remove();

        if(!(QFileInfo::exists(vsaveFilePath) && QFileInfo(vsaveFilePath).isFile())){
            qDebug() << vlink;
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
        qDebug() << "Lista vazia";
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
    QFile *tempFile = vfile;
    vfile = nullptr;
    tempFile->deleteLater();

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
    Q_UNUSED(value)
    m_busy = true;
    emit started();
    if(!vlistaSelecionada.isEmpty()){
        vlink = vlistaSelecionada[vindexLista]->vLinkImagemMedia;
        if(vlink.contains("medium"))
            vlink.replace("medium", "large");
        else if(vlink.contains("small"))
            vlink.replace("small", "large");
        vsaveFilePath = cconfBase->instance()->vdiretorioImagensGrandes;
        vsaveFilePath.append(vlistaSelecionada[vindexLista]->vid);
        vsaveFilePath.append(vlink.mid(vlink.lastIndexOf(QChar('.'))));
        if(QFile(vsaveFilePath).exists() && QFile(vsaveFilePath).size() == 0)
            QFile(vsaveFilePath).remove();

        if(!(QFileInfo::exists(vsaveFilePath) && QFileInfo(vsaveFilePath).isFile())){
             qDebug() << vsaveFilePath;
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
    QFile *tempFile = vfile;
    vfile = nullptr;
    tempFile->deleteLater();

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
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaWatching();
        else if(rlista.compare("COMPLETED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaCompleted();
        else if(rlista.compare("PAUSED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaOnHold();
        else if(rlista.compare("DROPPED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaDropped();
        else if(rlista.compare("PLANNING", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaPlanToWatch();
        else if(rlista.compare("SEARCH", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaBusca();
    }
    else if(rtipoLista.compare("MANGA", Qt::CaseInsensitive) == 0){
        if(rlista.compare("CURRENT", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaMangaReading();
        else if(rlista.compare("COMPLETED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaMangaCompleted();
        else if(rlista.compare("PAUSED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaMangaOnHold();
        else if(rlista.compare("DROPPED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaMangaDropped();
        else if(rlista.compare("PLANNING", Qt::CaseInsensitive) == 0)
                vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaMangaPlanToRead();
        else if(rlista.compare("SEARCH", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaBusca();
    }
    else if(rtipoLista.compare("NOVEL", Qt::CaseInsensitive) == 0){
        if(rlista.compare("CURRENT", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaNovelReading();
        else if(rlista.compare("COMPLETED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaNovelCompleted();
        else if(rlista.compare("PAUSED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaNovelOnHold();
        else if(rlista.compare("DROPPED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaNovelDropped();
        else if(rlista.compare("PLANNING", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaNovelPlanToRead();
        else if(rlista.compare("SEARCH", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaBusca();
    }
    else if(rtipoLista.compare("SEASON", Qt::CaseInsensitive) == 0){
        if(rlista.contains("WINTER", Qt::CaseInsensitive))
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaAnimeWinter();
        else if(rlista.contains("SPRING", Qt::CaseInsensitive))
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaAnimeSpring();
        else if(rlista.contains("SUMMER", Qt::CaseInsensitive))
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaAnimeSummer();
        else if(rlista.contains("FALL", Qt::CaseInsensitive))
            vlistaSelecionada = cleitorlistaanimes->instance()->retornaListaAnimeFall();
        else
            vlistaSelecionada = cleitorlistaanimes->instance()->fleListaAno(rlista.toInt());
    }
}

