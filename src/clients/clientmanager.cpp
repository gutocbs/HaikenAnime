#include "clientmanager.h"

ClientManager::ClientManager(QObject *parent) : QObject(parent)
{

}

void ClientManager::setUpdate()
{
    updateClient();
    //Recursão 30 segundos após finalizar a execução
    QTimer::singleShot(30, this, &ClientManager::setUpdate);
}

void ClientManager::downloadMediaList()
{

}

bool ClientManager::addToUpdateQueue(ClientEnums::updateType updateType, int mediaId, QVariant updatedValue)
{
}

void ClientManager::updateClient()
{
    ///TODO - Colocar a variável requestsPerSecondLimit em algum arquivo de configuração e ler de lá
    int requestsPerSecondLimit = 90;
    int resquestsDone = 0;
    QHash<int, Enums::mediaList>::iterator updateQueueListIterator;
    for (updateQueueListIterator = updateQueueList.begin(); updateQueueListIterator != updateQueueList.end(); ++updateQueueListIterator){
        if(client->updateList(updateQueueListIterator.key(), updateQueueListIterator.value()))
            updateQueueList.remove(updateQueueListIterator.key());
        if(++resquestsDone >= requestsPerSecondLimit)
            return;
    }
    QHash<int, int>::iterator updateQueueScoreIterator;
    for (updateQueueScoreIterator = updateQueueScore.begin(); updateQueueScoreIterator != updateQueueScore.end(); ++updateQueueScoreIterator){
        if(client->updateScore(updateQueueScoreIterator.key(), updateQueueScoreIterator.value()))
            updateQueueScore.remove(updateQueueScoreIterator.key());
        if(++resquestsDone >= requestsPerSecondLimit)
            return;
    }
    QHash<int, int>::iterator updateQueueProgressIterator;
    for (updateQueueProgressIterator = updateQueueProgress.begin(); updateQueueProgressIterator != updateQueueProgress.end(); ++updateQueueProgressIterator){
        if(client->updateProgress(updateQueueProgressIterator.key(), updateQueueProgressIterator.value()))
            updateQueueProgress.remove(updateQueueProgressIterator.key());
        if(++resquestsDone >= requestsPerSecondLimit)
            return;
    }
}
