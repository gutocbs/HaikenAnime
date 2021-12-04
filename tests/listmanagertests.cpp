#include "listmanagertests.h"

ListManagerTests::ListManagerTests(QObject *parent) : QObject(parent)
{

}

void ListManagerTests::initTestCase()
{
    this->animeListManager = new AnimeListManager(this);
    MediaLoader::loadMediaFromFile(true);
}

//TODO - Apagar a lista após os testes
void ListManagerTests::cleanupTestCase()
{

}

void ListManagerTests::getMediaList_WhenCalled_ReturnFullMediaList()
{
    auto mediaList = this->animeListManager->getInstance()->getMediaList(Enums::CURRENT);
    QVERIFY(mediaList.size() > 0);
    mediaList = this->animeListManager->getInstance()->getMediaList(Enums::COMPLETED);
    QVERIFY(mediaList.size() > 0);
    mediaList = this->animeListManager->getInstance()->getMediaList(Enums::DROPPED);
    QVERIFY(mediaList.size() > 0);
    mediaList = this->animeListManager->getInstance()->getMediaList(Enums::PAUSED);
    QVERIFY(mediaList.size() > 0);
    mediaList = this->animeListManager->getInstance()->getMediaList(Enums::PLANNING);
    QVERIFY(mediaList.size() > 0);
}
