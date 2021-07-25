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
MediaController::MediaController(QObject *parent) : QObject(parent)
{

}

void MediaController::initializeMedia()
{
    mediaListManager = new AnimeListManager(this);
    mediaManager = new AnimeManager(this);
    mediaSearchManager = new MediaSearchManager(this);
}

//Criar funções de controle de mangas e novels
void MediaController::setMediaManager(Enums::mediaType mediaType)
{
    switch (mediaType) {
    case Enums::ANIME:
        mediaManager = new AnimeManager(this);
        break;
    case Enums::MANGA:
//        mediaManager = new AnimeManager(this);
        break;
    case Enums::NOVEL:
//        mediaManager = new AnimeManager(this);
        break;
    }
}

void MediaController::setMediaListManager(Enums::mediaType mediaType)
{
    switch (mediaType) {
    case Enums::ANIME:
        mediaListManager = new AnimeListManager(this);
        break;
    case Enums::MANGA:
//        mediaListManager = new AnimeListManager(this);
        break;
    case Enums::NOVEL:
//        mediaListManager = new AnimeListManager(this);
        break;
    }
}

QPointer<IMediaListManager> MediaController::getMediaListManager()
{
    return mediaListManager;
}

QPointer<IMediaSearchManager> MediaController::getMediaSearchManager()
{
    return mediaSearchManager;
}

QPointer<IMediaManager> MediaController::getMediaManager()
{
    return mediaManager;
}
