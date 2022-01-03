#include "mediasearchmanager.h"

//int MediaSearchManager::buscaIDRetornaPosicao(int idMedia)
//{
//    int idMedia = -1;
////Passar MediaSearchManager pelo construtor. Fazer uma classe que irá setar todos as classes por tipos

//    return id;
//}

MediaSearchManager::MediaSearchManager(QObject *parent, IMediaListManager* mediaListManager) : IMediaSearchManager(parent)
{
    this->mediaListManager = mediaListManager;
}

void MediaSearchManager::setMediaListManager(IMediaListManager* mediaListManager){
    this->mediaListManager = mediaListManager;
}

void MediaSearchManager::loadListNames()
{
    listNames.append("CURRENT");
    listNames.append("COMPLETED");
    listNames.append("PLANNING");
    listNames.append("DROPPED");
    listNames.append("PAUSED");
}

QVector<Media*> MediaSearchManager::getMediaList(const QString &listName){
    Enums::mediaList listEnum = Enums::QStringToMediaList(listName);
    return mediaListManager->getInstance()->getMediaList(listEnum);
}

QPointer<Media> MediaSearchManager::getMediaFromListIndex(Enums::mediaList list, int posicao)
{

}

QStringList MediaSearchManager::getNamesById(int id)
{
    QHash<int, Media*> hashMediaById = mediaListManager->getInstance()->getHashMediaById();
    QStringList names;
    if(hashMediaById.contains(id)){
        names.append(hashMediaById.value(id)->customNames);
        names.insert(0,hashMediaById.value(id)->englishName);
        names.insert(0,hashMediaById.value(id)->originalName);
    }
    return names;
}

//TODO - Buscar nas listas de anos também
bool MediaSearchManager::searchMedia(const QString &rnome)
{
    QVector<Media*> mediaListSearch = mediaListManager->getInstance()->getMediaList(Enums::SEARCH);
    if(rnome.contains(QRegExp("\\S"))){
        mediaListSearch.clear();
        QHash<int, Media*> hashMediaById = mediaListManager->getInstance()->getHashMediaById();
        QHash<int, Media*>::iterator iterator;
        for (iterator = hashMediaById.begin(); iterator != hashMediaById.end(); ++iterator){
            QStringList names = getNamesById(hashMediaById[iterator.key()]->id);
            for (const auto& name : names )
            {
                if(name.contains(rnome, Qt::CaseInsensitive)){
                    mediaListSearch.append(hashMediaById[iterator.key()]);
                    break;
                }
            }
        }
    }
    bool hasMedia = mediaListSearch.isEmpty() ? false : true;
    if(hasMedia)
        mediaListManager->getInstance()->setMediaList(Enums::mediaList::SEARCH, mediaListSearch);
    return hasMedia;
}

void MediaSearchManager::appendToList(QVector<Media*> &mediaList, Enums::mediaList list, int position){
    QVector<Media*> tempList = mediaListManager->getInstance()->getMediaList(list);
    if(tempList.size() > position)
        mediaList.append(tempList[position]);
}

int MediaSearchManager::buscaIDRapido(const QString &rnomemedia)
{
    QHash<int, Media*> hashMediaById = mediaListManager->getInstance()->getHashMediaById();

    QHash<int, Media*>::iterator iterator;
    for (iterator = hashMediaById.begin(); iterator != hashMediaById.end(); ++iterator){
        QStringList names = getNamesById(hashMediaById[iterator.key()]->id);
        if(names.contains(rnomemedia))
            return iterator.key();
    }
    return -1;
}

QPointer<Media> MediaSearchManager::getMediaFromId(int idMedia){
    QHash<int, Media*> hashMediaById = mediaListManager->getInstance()->getHashMediaById();
    if(hashMediaById.contains(idMedia))
        return hashMediaById.value(idMedia);
    return nullptr;
}

QVector<Media*> MediaSearchManager::getMediaListFromId(int idMedia){
    QPointer<Media> media = getMediaFromId(idMedia);
    if(!media.isNull()){
        QVector<Media*> tempList = mediaListManager->getInstance()->getMediaList(media->mediaList);
        return tempList;
    }
    return QVector<Media*>();
}

int MediaSearchManager::getMediaEpisodeFromId(int idMedia)
{
    QPointer<Media> media = getMediaFromId(idMedia);
    if(!media.isNull())
        return media->progress;
    return 0;
}

QString MediaSearchManager::getMediaScoreFromId(int idMedia)
{
    QPointer<Media> media = getMediaFromId(idMedia);
    if(!media.isNull())
        return media->personalScore;
    return "0";
}

QString MediaSearchManager::getMediaTitleFromId(int idMedia)
{
    QPointer<Media> media = getMediaFromId(idMedia);
    if(!media.isNull())
        return media->originalName;
    return "0";
}

int MediaSearchManager::getIdFromMediaTitle(const QString &mediaTitle)
{
    //Verifica se o titulo é válido, ou seja, não é só um espaço em branco
    if(mediaTitle.contains(QRegExp("\\S"))){
        QHash<int, Media*> hashMediaById = mediaListManager->getInstance()->getHashMediaById();
        QHash<int, Media*>::iterator iterator;
        for (iterator = hashMediaById.begin(); iterator != hashMediaById.end(); ++iterator){
            QStringList names = getNamesById(hashMediaById[iterator.key()]->id);
            if(names.contains(mediaTitle, Qt::CaseInsensitive))
                return iterator.key();
        }
    }
    return 0;
}


//TODO - Fazer função
QPointer<Media> MediaSearchManager::fbuscaMediaNoAno(int ano, int id)
{
    return QPointer<Media>();
}
