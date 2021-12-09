#include "mediaplayer.h"

MediaPlayer::MediaPlayer(QObject *parent) : QObject(parent)
{
    QPointer<MediaController> mediaController = new MediaController(this);
    mediaSearchManager = mediaController->instance()->getMediaSearchManager();
}

QPointer<CurrentMediaPlaying> MediaPlayer::getCurrentMediaPlaying()
{
    //Crio uma variável da lib Robot para monitorar as janelas abertas
    QScopedPointer<Robot::Window> window(new Robot::Window);
    std::vector<Robot::Window> windowList = window->GetList();
    std::vector<Robot::Window>::iterator windowIterator;
    QString windowName;

    for(windowIterator = windowList.begin(); windowIterator != windowList.end(); windowIterator++){
        windowName = getCleanWindowName(windowIterator->GetTitle());
        if(!windowName.isEmpty() && compareWindowNameToMediaPlayer(windowName)){
            int mediaId = mediaSearchManager->getIdFromMediaTitle(getMediaName(windowName));
            QPointer<Media> media = mediaSearchManager->getMediaFromId(mediaId);
            if(!media.isNull()){
                return new CurrentMediaPlaying(media->id, media->originalName, getEpisode(windowName));
            }
        }
    }
    return nullptr;
}

QString MediaPlayer::getCleanWindowName(std::string windowName)
{
    QString name = QString::fromStdString(windowName);
    return name;
}

QString MediaPlayer::getMediaName(QString windowName)
{

}

int MediaPlayer::getEpisode(QString windowName)
{

}

bool MediaPlayer::compareWindowNameToMediaPlayer(QString windowName)
{
    //Pego o nome de todos os players selecionados nas configurações para reconhecimento
    //TODO - Pegar lista de players nas configurações
    //    QStringList mediaPLayers = cabaConfig->instance()->fgetPlayers();
    QStringList mediaPlayers("meida");
    QList<QString>::const_iterator mediaPlayersIterator;
    for (mediaPlayersIterator = mediaPlayers.constBegin(); mediaPlayersIterator != mediaPlayers.constEnd(); ++mediaPlayersIterator){
        if(windowName.contains(*mediaPlayersIterator, Qt::CaseInsensitive))
            return true;
    }
    return false;
}
