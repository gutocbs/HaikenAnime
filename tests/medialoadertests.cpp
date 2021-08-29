#include "medialoadertests.h"

MediaLoaderTests::MediaLoaderTests(QObject *parent) : QObject(parent)
{

}

void MediaLoaderTests::initTestCase()
{
    this->animeListManager = animeListManager->instance();
    this->mediaLoader = new MediaLoader(this,animeListManager);

}

void MediaLoaderTests::cleanupTestCase()
{

}

void MediaLoaderTests::loadMediaFromFile_WhenCalled_returnTrue()
{
    auto sucess = this->mediaLoader->loadMediaFromFile(true);
    QVERIFY(sucess);
}

void MediaLoaderTests::loadMediaFromFile_WhenCalled_returnFalse()
{
    auto sucess = this->mediaLoader->loadMediaFromFile();
    QVERIFY(sucess);
}
