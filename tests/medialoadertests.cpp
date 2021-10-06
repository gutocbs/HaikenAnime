#include "medialoadertests.h"

MediaLoaderTests::MediaLoaderTests(QObject *parent) : QObject(parent)
{

}

void MediaLoaderTests::initTestCase()
{
    this->animeListManager = animeListManager->instance();
    //    this->mediaLoader = new MediaLoader(this);

}

void MediaLoaderTests::cleanupTestCase()
{

}

void MediaLoaderTests::loadMediaFromFile_WhenCalled_returnTrue()
{
    auto sucess = MediaLoader::loadMediaFromFile(true);
    QVERIFY(sucess);
}

void MediaLoaderTests::loadMediaFromFile_WhenCalled_returnFalse()
{
    auto sucess = MediaLoader::loadMediaFromFile();
    QVERIFY(sucess);
}
//QTEST_MAIN(MediaLoaderTests)
