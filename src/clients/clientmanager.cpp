#include "clientmanager.h"

ClientManager::ClientManager(QObject *parent) : QObject(parent)
{
}

ClientManager::~ClientManager()
{
}

void ClientManager::setClient(ClientEnums::clients clientEnum)
{
    switch (clientEnum) {
    case ClientEnums::ANILIST:
        client = new anilist();
        break;
    }

}

void ClientManager::setAuthCode(const QString &user, QVariant authcode)
{
    client->setAuthCode(user, authcode);
}

void ClientManager::setUpdate()
{
    updateClient();
    //Recursão 30 segundos após finalizar a execução
    QTimer::singleShot(30, this, &ClientManager::setUpdate);
}

void ClientManager::downloadMediaList()
{
    QFuture<void> downloadMedia = QtConcurrent::run([=]() {
        client->getAvatarAndMediaList();
    });
}

void ClientManager::downloadYearlyLists()
{
    client->getYearlyLists();
}

QString ClientManager::getAvatar()
{
    return client->getAvatar();
}

void ClientManager::setConnections()
{
    ClientEnums::clients clientEnum = ClientEnums::QStringToClient(client->metaObject()->className());
    switch (clientEnum) {
    case ClientEnums::ANILIST:
        connect(dynamic_cast<anilist*>(client.data()), &anilist::downloadFinished, this, &ClientManager::sendDownloadSignal);
        break;
    }
}

void ClientManager::sendDownloadSignal(bool signal)
{
    emit signalDownloadCompleted(signal);
}

bool ClientManager::addToUpdateQueue(ClientEnums::updateType updateType, int mediaId, QVariant updatedValue)
{
    switch (updateType) {
    case ClientEnums::SCORE:
        updateQueueScore.insert(mediaId, updatedValue.toInt());
        break;
    case ClientEnums::PROGRESS:
        updateQueueProgress.insert(mediaId, updatedValue.toInt());
        break;
    case ClientEnums::LIST:
        updateQueueList.insert(mediaId, Enums::QStringToMediaList(updatedValue.toString()));
        break;
    case ClientEnums::DELETE:
        updateQueueList.insert(mediaId, Enums::mediaList::NOLIST);
        break;

    }
    return true;
}

void ClientManager::updateClient()
{
    ///TODO - Colocar a variável requestsPerSecondLimit em algum arquivo de configuração e ler de lá
    int requestsPerSecondLimit = 90;
    int resquestsDone = 0;
    if(!updateQueueList.isEmpty()){
        QHash<int, Enums::mediaList>::iterator updateQueueListIterator;
        for (updateQueueListIterator = updateQueueList.begin(); updateQueueListIterator != updateQueueList.end(); updateQueueListIterator++){
            if(updateQueueListIterator.value() != Enums::mediaList::NOLIST && client->updateList(updateQueueListIterator.key(), updateQueueListIterator.value()))
                updateQueueList.remove(updateQueueListIterator.key());
            else if(updateQueueListIterator.value() == Enums::mediaList::NOLIST && client->deleteMediaFromList(updateQueueListIterator.key()))
                updateQueueList.remove(updateQueueListIterator.key());
            if(++resquestsDone >= requestsPerSecondLimit)
                return;
        }
    }
    if(!updateQueueScore.isEmpty()){
        QHash<int, int>::iterator updateQueueScoreIterator;
        for (updateQueueScoreIterator = updateQueueScore.begin(); updateQueueScoreIterator != updateQueueScore.end(); updateQueueScoreIterator++){
            if(client->updateScore(updateQueueScoreIterator.key(), updateQueueScoreIterator.value()))
                updateQueueScore.erase(updateQueueScoreIterator);
            if(++resquestsDone >= requestsPerSecondLimit)
                return;
        }
    }
    if(!updateQueueProgress.isEmpty()){
        QHash<int, int>::iterator updateQueueProgressIterator = updateQueueProgress.begin();
        while (updateQueueProgressIterator != updateQueueProgress.end()) {
            QHash<int, int>::iterator prev = updateQueueProgressIterator;
            ++updateQueueProgressIterator;
            if(client->updateProgress(prev.key(), prev.value()))
                updateQueueProgress.erase(prev);
            if(++resquestsDone >= requestsPerSecondLimit)
                return;
        }
    }
}
