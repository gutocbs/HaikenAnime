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
    dw->setObjectName("DownloadWorker " + QString::number(workers.size()));
    connect(dw,&DownloadWorker::downloadFinished,this, &DownloadManager::checkwork);
    workers.append(dw);
}

int DownloadManager::getWorkersNumber()
{
    return workers.size();
}

void DownloadManager::setWork(QVariant value)
{
    QString workType = value.typeName();
    if(workType.compare("QString", Qt::CaseInsensitive) == 0)
        workVector.append(value);
    else if(workType.compare("int", Qt::CaseInsensitive) == 0)
        workVector.append(value);
    //TODO - Else dar um log aqui
}

//Adiciona um trabalho na fila para que um worker possa pegar
void DownloadManager::work()
{
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

//TODO - Fazer os downloads pararem quando fechar o programa
void DownloadManager::setDownload(DownloadWorker* dw)
{
    QVariant work = workVector.takeFirst();
    QString workType = work.typeName();
    //Caso seja um link, baixar direto o link
    if(workType.compare("QString", Qt::CaseInsensitive) == 0)
        dw->download(DownloadEnums::Torrent, work.toString());
    //Caso seja um id, buscar a media certa e baixar
    else if(workType.compare("int", Qt::CaseInsensitive) == 0){
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
    QPointer<MediaController> mediaController = new MediaController();
    QPointer<IMediaSearchManager> mediaSearchManager = mediaController->instance()->getMediaSearchManager();
    QPointer<IMediaListManager> listManager;
    QPointer<Media> media;
    while(media.isNull()){
        if(media.isNull() && listManager.isNull())
            listManager = mediaController->instance()->getMediaListManager(Enums::mediaType::ANIME);
        else if(media.isNull() && listManager->getMediaType() == Enums::mediaType::ANIME)
            listManager = mediaController->instance()->getMediaListManager(Enums::mediaType::MANGA);
        else if(media.isNull() && listManager->getMediaType() == Enums::mediaType::MANGA)
            listManager = mediaController->instance()->getMediaListManager(Enums::mediaType::NOVEL);
        else
            break;
        //TODO - Testar instance em vez de testes
        mediaSearchManager->setMediaListManager(listManager->getInstance());
        media = mediaSearchManager->getMediaFromId(id);
    }
    return media;
}



