#include "downloader.h"

Downloader::Downloader(QObject *parent) : QObject(parent)
{
    dw = new DownloaderWorker(this);
}

Downloader::~Downloader()
{
    if(!m_workers.isEmpty()){
        qDeleteAll(m_workers);
        m_workers.clear();
    }
}

void Downloader::fsetWorker()
{
    dw->setObjectName("DownloaderWorker: " + QString::number(0));
    connect(dw,&DownloaderWorker::finished,this, &Downloader::finished);
    connect(dw,&DownloaderWorker::finishedBig,this, &Downloader::finishedBig);
    connect(dw,&DownloaderWorker::finishedXML,this, &Downloader::finishedXML);

    m_workers.append(dw);
}

void Downloader::setListAndType(QString rlista, QString rtipoLista)
{
    qDebug() << rlista << rtipoLista;
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

bool Downloader::isBusy()
{
    if(dw->isBusy())
        return true;
    return false;
}

