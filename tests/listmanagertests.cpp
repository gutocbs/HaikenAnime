#include "listmanagertests.h"

ListManagerTests::ListManagerTests(QObject *parent) : QObject(parent)
{

}

void ListManagerTests::initTestCase()
{
    this->animeListManager = animeListManager->instance();
    this->mediaLoader = new MediaLoader(this,animeListManager);
    this->mediaLoader->loadMediaFromFile(true);
}

//TODO - Apagar a lista após os testes
void ListManagerTests::cleanupTestCase()
{

}

void ListManagerTests::getMediaList_WhenCalled_ReturnFullMediaList()
{
    auto mediaList = this->animeListManager->getMediaList(Enums::CURRENT);
    QVERIFY(mediaList.size() > 0);
    mediaList = this->animeListManager->getMediaList(Enums::COMPLETED);
    QVERIFY(mediaList.size() > 0);
    mediaList = this->animeListManager->getMediaList(Enums::DROPPED);
    QVERIFY(mediaList.size() > 0);
    mediaList = this->animeListManager->getMediaList(Enums::PAUSED);
    QVERIFY(mediaList.size() > 0);
    mediaList = this->animeListManager->getMediaList(Enums::PLANNING);
    QVERIFY(mediaList.size() > 0);
}

void ListManagerTests::getHashMediaIndexById_WhenCalled_ReturnFullhashMediaIndexById()
{
    auto hashMediaIndexById = this->animeListManager->getHashMediaIndexById();
    QVERIFY(hashMediaIndexById.size() > 0);
}

void ListManagerTests::getHashMediaListById_WhenCalled_ReturnFullhashMediaListById()
{
    auto hashMediaListById = this->animeListManager->getHashMediaListById();
    QVERIFY(hashMediaListById.size() > 0);
}

void ListManagerTests::getHashMediaNamesById_WhenCalled_ReturnFullhashMediaNamesById()
{
    auto hashMediaNamesByIdNomeAlternativo = this->animeListManager->getHashMediaNamesById(Enums::NOMEALTERNATIVO);
    auto hashMediaNamesByIdNome = this->animeListManager->getHashMediaNamesById(Enums::NOME);
    QVERIFY(hashMediaNamesByIdNomeAlternativo.size() > 0 && hashMediaNamesByIdNome.size() > 0 );
}
