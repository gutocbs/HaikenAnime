#include "downloadmanager.h"
//Fazer um singleton
DownloadManager *DownloadManager::createInstance()
{

    return new DownloadManager();
}

DownloadManager *DownloadManager::instance()
{
    return Singleton<DownloadManager>::instance(DownloadManager::createInstance);
}

DownloadManager::DownloadManager(QObject *parent) : QObject(parent)
{
}

//Cria um novo worker e o coloca na fila de trabalho
void DownloadManager::setWorker()
{
    QPointer<DownloadWorker> dw = new DownloadWorker(this);
    dw->setObjectName("DownloadWorker " + QString::number(0));
    workers.append(dw);
}

int DownloadManager::getWorkersNumber()
{
    return workers.size();
}

void DownloadManager::work(QVariant value)
{
    QString workType = value.typeName();
    if(workType.compare("QString", Qt::CaseInsensitive) == 0)
        work(value.toString());
    else if(workType.compare("integrer", Qt::CaseInsensitive) == 0)
        work(value.toInt());
}

//Adiciona um trabalho na fila para que um worker possa pegar
void DownloadManager::work(int value)
{
    workVector.append(value);
    checkwork();
}

void DownloadManager::work(QString value)
{
    workVector.append(value);
    checkwork();
}

//Verifica a lista de trabalhos e distribui caso possível
void DownloadManager::checkwork()
{
    if(workVector.isEmpty()) return;
    foreach(DownloadWorker *dw, workers) {
        //Caso o worker esteja disponível, fazer trabalhar
        if(!dw->isBusy()) {
            setDownload(dw);
            if(workVector.isEmpty()) return;
        }
    }
}

void DownloadManager::setDownload(DownloadWorker* dw)
{
    QVariant work = workVector.takeFirst();
    QString workType = work.typeName();
    //Caso seja um link, baixar direto o link
    if(workType.compare("QString", Qt::CaseInsensitive) == 0)
        dw->download(DownloadEnums::Torrent, work.toString());
    //Caso seja um id, buscar a media certa e baixar
    else if(workType.compare("integrer", Qt::CaseInsensitive) == 0){
        QPointer<Media> media = getMedia(work.toInt());
        if(!media.isNull()){
            dw->download(DownloadEnums::Cover, media->coverURL,DownloadEnums::Medium);
            dw->download(DownloadEnums::Cover, media->coverURL,DownloadEnums::Big);
        }
    }
    checkwork();
}

QPointer<Media> DownloadManager::getMedia(int id)
{
    QPointer<MediaSearchManager> mediaSearchManager = new MediaSearchManager();
    QPointer<IMediaListManager> listManager;
    QPointer<Media> media;
    while(media.isNull() || listManager->getMediaType() != Enums::mediaType::NOVEL){
        if(media.isNull() && listManager.isNull())
            listManager = new AnimeListManager();
        else if(media.isNull() && listManager->getMediaType() == Enums::mediaType::ANIME)
            listManager = new MangaListManager();
        else if(media.isNull() && listManager->getMediaType() == Enums::mediaType::MANGA)
            listManager = new NovelListManager();
        mediaSearchManager->setMediaListManager(listManager->instance());
        media = mediaSearchManager->getMediaFromId(id);
    }
    return media;
}



