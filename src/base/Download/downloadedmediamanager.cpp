#include "downloadedmediamanager.h"

DownloadedMediaManager::DownloadedMediaManager(QObject *parent) : QObject(parent)
{

}

//Isso deveria ser uma nova classe: DownloadConfigurations
bool DownloadedMediaManager::mediaExists(const int &mediaId)
{
    bool mediaExists{false};
    if(downloadedMedia.contains(mediaId))
        mediaExists = true;
    return mediaExists;
}

void DownloadedMediaManager::readDownloadedMediaFiles()
{
    //Ler o arquivo de configuração de downloaads para ver quais arquivos já foram baixados
    //Salvar arquivo na hash downloadedMedia
    //Sempre que baixar alguma midia, salvar no hash
}
