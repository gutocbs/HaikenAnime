#include "downloader.h"

Downloader::Downloader(QObject *parent) : QObject(parent)
{
    dw = new DownloaderWorker(this);
}

Downloader::~Downloader()
{
}

void Downloader::fsetWorker()
{
    dw->setObjectName("DownloaderWorker: " + QString::number(0));
    connect(dw,&DownloaderWorker::finished,this, &Downloader::finished);
    connect(dw,&DownloaderWorker::finishedBig,this, &Downloader::finishedBig);
    connect(dw,&DownloaderWorker::finishedXML,this, &Downloader::finishedXML);
    connect(dw,&DownloaderWorker::finishedSpecificXML,this, &Downloader::finishedSpecificXML);
    connect(dw,&DownloaderWorker::finishedAnimeTorrent,this, &Downloader::finishedAnimeTorrent);

    m_workers.append(dw);
}

void Downloader::setListAndType(Enums::mediaList mediaList, Enums::mediaType mediaType)
{
    dw->getList(mediaList, mediaType);
}

void Downloader::setListAndType(QString rlista, QString rtipoLista)
{
    dw->fselecionaLista(rlista, rtipoLista);
}

void Downloader::setAvatar(QString urlAvatar)
{
    dw->fsetURLAvatar(urlAvatar);
}


void Downloader::work(int value)
{
    m_work.append(value);
    checkwork();
}

void Downloader::finished()
{
    checkwork();
}

void Downloader::checkwork()
{
    if(m_work.isEmpty()) return;
    foreach(DownloaderWorker *dw, m_workers) {
        if(!dw->isBusy()) {
            dw->work(m_work.takeFirst());
            if(m_work.isEmpty()) return;
        }
    }
}

void Downloader::workBig(int value)
{
    m_work.append(value);
    checkworkBig();
}

void Downloader::finishedBig()
{
    checkworkBig();
}

void Downloader::checkworkBig()
{
    if(m_work.isEmpty()) return;
    foreach(DownloaderWorker *dw, m_workers) {
        if(!dw->isBusy()) {
            dw->workBig(m_work.takeFirst());
            if(m_work.isEmpty()) return;
        }
    }
}

void Downloader::workAvatar(int value)
{
    m_work.append(value);
    checkworkAvatar();
}

void Downloader::finishedAvatar()
{
    checkworkAvatar();
}

void Downloader::checkworkAvatar()
{
    if(m_work.isEmpty()) return;
    foreach(DownloaderWorker *dw, m_workers) {
        if(!dw->isBusy()) {
            dw->fdownloadAvatarUsuario(m_work.takeFirst());
            if(m_work.isEmpty()) return;
        }
    }
}

void Downloader::workXML(int value)
{
    m_work.append(value);
    checkworkXML();
}

void Downloader::finishedXML()
{
    checkworkXML();
    emit sfinishedXML();
}

void Downloader::checkworkXML()
{
    if(m_work.isEmpty()) return;
    foreach(DownloaderWorker *dw, m_workers) {
        if(!dw->isBusy()) {
            dw->fdownloadXMLTorrentList();
            m_work.takeFirst();
            if(m_work.isEmpty()) return;
        }
    }
}

void Downloader::workSpecificXML(int value, QString search)
{
    searchName = search;
    m_work.append(value);
    checkworkSpecificXML();
}

void Downloader::finishedSpecificXML()
{
    checkworkSpecificXML();
    emit sfinishedXML();
}

void Downloader::checkworkSpecificXML()
{
    if(m_work.isEmpty()) return;
    foreach(DownloaderWorker *dw, m_workers) {
        if(!dw->isBusy()) {
            dw->fdownloadSpecificXMLTorrentList(searchName);
            m_work.takeFirst();
            if(m_work.isEmpty()) return;
        }
    }
}

void Downloader::workAnimeTorrent(int value, QString link, QString nome)
{
    torrentLink.append(link);
    torrentName.append(nome);
    m_work.append(value);
    checkworkAnimeTorrent();
}

void Downloader::finishedAnimeTorrent()
{
    checkworkAnimeTorrent();
    emit sfinishedAnimeTorrent();
}

void Downloader::checkworkAnimeTorrent()
{
    if(m_work.isEmpty()) return;
    foreach(DownloaderWorker *dw, m_workers) {
        if(!dw->isBusy() && !torrentLink.isEmpty() && !torrentName.isEmpty()) {
            dw->fdownloadAnimeTorrent(torrentLink.takeFirst(), torrentName.takeFirst(), m_work.first());
            m_work.takeFirst();
            if(m_work.isEmpty()) return;
        }
    }
}

bool Downloader::isBusy()
{
    if(dw->isBusy())
        return true;
    return false;
}

