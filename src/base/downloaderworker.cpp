#include "downloaderworker.h"

DownloaderWorker::DownloaderWorker(QObject *parent) : QObject(parent)
{
    vmanager = new QNetworkAccessManager(this);
    vreply = nullptr;
    vfileIsOpen = false;
    vindexLista = 0;
    vfile = nullptr;
    m_busy = false;
    cconfBase = new confBase;
    cdatabase = new Database;
    //    cdatabase->instance();
}

DownloaderWorker::~DownloaderWorker()
{
    vreply->close();
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
        if(!cdatabase->instance()->fchecaDatabaseReady())
            return;
        animeSelecionado = cdatabase->instance()->fretornaAnimePorPosicao(vlista, vindexLista);
        vlink = animeSelecionado->vLinkImagemMedia;
        if(vlink.contains("large"))
            vlink.replace("large", "medium");
        else if(vlink.contains("small"))
            vlink.replace("small", "medium");
        vsaveFilePath = cconfBase->instance()->vdiretorioImagensMedio;
        vsaveFilePath.append(animeSelecionado->vid);
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
        m_busy = false;
        emit finished();
    }
}

void DownloaderWorker::ffinished()
{
//    if(vdownloadPequeno)
//        emit sidGrande(animeSelecionado->vid);
//    else
//        emit sid(animeSelecionado->vid);
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
        if(!cdatabase->instance()->fchecaDatabaseReady())
            return;
        animeSelecionado = cdatabase->instance()->fretornaAnimePorPosicao(vlista, vindexLista);
        vlink = animeSelecionado->vLinkImagemMedia;
        if(vlink.contains("medium"))
            vlink.replace("medium", "large");
        else if(vlink.contains("small"))
            vlink.replace("small", "large");
        vsaveFilePath = cconfBase->instance()->vdiretorioImagensGrandes;
        vsaveFilePath.append(animeSelecionado->vid);
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
    vlista = rlista;
    if(rtipoLista.compare("ANIME", Qt::CaseInsensitive) == 0){
        if(rlista.compare("CURRENT", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnAnimeList("CURRENT");
        else if(rlista.compare("COMPLETED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnAnimeList("COMPLETED");
        else if(rlista.compare("PAUSED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnAnimeList("PAUSED");
        else if(rlista.compare("DROPPED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnAnimeList("DROPPED");
        else if(rlista.compare("PLANNING", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnAnimeList("PLANNING");
        else if(rlista.compare("SEARCH", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnAnimeList("SEARCH");
    }
    else if(rtipoLista.compare("MANGA", Qt::CaseInsensitive) == 0){
        if(rlista.compare("CURRENT", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnMangaList("CURRENT");
        else if(rlista.compare("COMPLETED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnMangaList("COMPLETED");
        else if(rlista.compare("PAUSED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnMangaList("PAUSED");
        else if(rlista.compare("DROPPED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnMangaList("DROPPED");
        else if(rlista.compare("PLANNING", Qt::CaseInsensitive) == 0)
                vlistaSelecionada = cdatabase->instance()->returnMangaList("PLANNING");
        else if(rlista.compare("SEARCH", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnMangaList("SEARCH");
    }
    else if(rtipoLista.compare("NOVEL", Qt::CaseInsensitive) == 0){
        if(rlista.compare("CURRENT", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnNovelList("CURRENT");
        else if(rlista.compare("COMPLETED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnNovelList("COMPLETED");
        else if(rlista.compare("PAUSED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnNovelList("PAUSED");
        else if(rlista.compare("DROPPED", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnNovelList("DROPPED");
        else if(rlista.compare("PLANNING", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnNovelList("PLANNING");
        else if(rlista.compare("SEARCH", Qt::CaseInsensitive) == 0)
            vlistaSelecionada = cdatabase->instance()->returnNovelList("SEARCH");
    }
    else if(rtipoLista.compare("SEASON", Qt::CaseInsensitive) == 0){
        if(rlista.contains("WINTER", Qt::CaseInsensitive))
            vlistaSelecionada = cdatabase->instance()->returnAnimeSeasonalList("WINTER");
        else if(rlista.contains("SPRING", Qt::CaseInsensitive))
            vlistaSelecionada = cdatabase->instance()->returnAnimeSeasonalList("SPRING");
        else if(rlista.contains("SUMMER", Qt::CaseInsensitive))
            vlistaSelecionada = cdatabase->instance()->returnAnimeSeasonalList("SUMMER");
        else if(rlista.contains("FALL", Qt::CaseInsensitive))
            vlistaSelecionada = cdatabase->instance()->returnAnimeSeasonalList("FALL");
        else
            vlistaSelecionada = cdatabase->instance()->returnAnimeYearlyList(rlista.toInt());
    }
}

