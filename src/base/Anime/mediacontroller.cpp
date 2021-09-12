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
    switch (mediaType) {
    case Enums::ANIME:
        mediaManager = animeManager->instance();
        break;
    case Enums::MANGA:
        mediaManager = mangaManager->instance();
        break;
    case Enums::NOVEL:
        mediaManager = novelManager->instance();
        break;
    }
}

void MediaController::setMediaListManager(Enums::mediaType mediaType)
{
    switch (mediaType) {
    case Enums::ANIME:
        mediaListManager = animeListManager->instance();
        break;
    case Enums::MANGA:
        mediaListManager = mangaListManager->instance();
        break;
    case Enums::NOVEL:
        mediaListManager = novelListManager->instance();
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

QPointer<IMediaSearchManager> MediaController::getMediaSearchManager()
{
    return mediaSearchManager;
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
