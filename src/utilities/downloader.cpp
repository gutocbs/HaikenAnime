#include "downloader.h"

Downloader::Downloader(QObject *parent) : QObject(parent)
{
    dw = new DownloaderWorker(this);
}

Downloader::~Downloader()
{
    qDeleteAll(m_workers);
    m_workers.clear();
}

void Downloader::fsetPointers(leitorlistaanimes *rleitorlistaanimes, confBase*rconfbase)
{
    dw->setObjectName("DownloaderWorker: " + QString::number(0));
    dw->fsetLeitorListaAnimes(rleitorlistaanimes);
    dw->fsetConfBase(rconfbase);
    connect(dw,&DownloaderWorker::finished,this, &Downloader::finished);
    connect(dw,&DownloaderWorker::finishedBig,this, &Downloader::finishedBig);

    m_workers.append(dw);
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

bool Downloader::isBusy()
{
    if(dw->isBusy())
        return true;
    return false;
}

