#include "mediacontroller.h"

//TODO - Terminar classe
MediaController *MediaController::createInstance()
{
    return new MediaController();
}

MediaController *MediaController::instance()
{
    return Singleton<MediaController>::instance(MediaController::createInstance);
}

//Todo - Chamar a função de carregar a lista
//Sempre pegar as medias daqui, nunca criar novas
MediaController::MediaController(QObject *parent) : QObject(parent)
{
}

void MediaController::initializeMedia()
{
    mediaListManager = new AnimeListManager(this);
    mediaManager = new AnimeManager(this);
    mediaSearchManager = new MediaSearchManager(this);
    mediaSearchManager->setMediaListManager(mediaListManager);
    animeListManager = new AnimeListManager(this);
    mangaListManager = new MangaListManager(this);
    novelListManager = new NovelListManager(this);
    animeManager = new AnimeManager(this);
    mangaManager = new MangaManager(this);
    novelManager = new NovelManager(this);
}

//Criar funções de controle de mangas e novels
void MediaController::setMediaManager(Enums::mediaType mediaType)
{
    if(animeManager == nullptr)
        initializeMedia();
    switch (mediaType) {
    case Enums::ANIME:
        mediaManager = animeManager->getInstance();
        break;
    case Enums::MANGA:
        mediaManager = mangaManager->getInstance();
        break;
    case Enums::NOVEL:
        mediaManager = novelManager->getInstance();
        break;
    }
}

void MediaController::setMediaListManager(Enums::mediaType mediaType)
{
    if(animeListManager == nullptr)
        initializeMedia();
    switch (mediaType) {
    case Enums::ANIME:
        mediaListManager = animeListManager->getInstance();
        break;
    case Enums::MANGA:
        mediaListManager = mangaListManager->getInstance();
        break;
    case Enums::NOVEL:
        mediaListManager = novelListManager->getInstance();
        break;
    }
}

QPointer<IMediaListManager> MediaController::getMediaListManager(Enums::mediaType mediaType)
{
    switch (mediaType) {
    case Enums::ANIME:
        setMediaListManager(Enums::ANIME);
        break;
    case Enums::MANGA:
        setMediaListManager(Enums::MANGA);
        break;
    case Enums::NOVEL:
        setMediaListManager(Enums::NOVEL);
        break;
    }
    return mediaListManager;
}

QPointer<IMediaManager> MediaController::getMediaManager(Enums::mediaType mediaType)
{
    switch (mediaType) {
    case Enums::ANIME:
        setMediaManager(Enums::ANIME);
        break;
    case Enums::MANGA:
        setMediaManager(Enums::MANGA);
        break;
    case Enums::NOVEL:
        setMediaManager(Enums::NOVEL);
        break;
    }
    return mediaManager;
}

QPointer<IMediaSearchManager> MediaController::getMediaSearchManager()
{
    return mediaSearchManager;
}

