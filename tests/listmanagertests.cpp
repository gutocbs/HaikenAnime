#include "listmanagertests.h"

ListManagerTests::ListManagerTests(QObject *parent) : QObject(parent)
{

}

void ListManagerTests::initTestCase()
{
    this->animeListManager = animeListManager->instance();
    MediaLoader::loadMediaFromFile(true);
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
