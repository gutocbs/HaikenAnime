#include "testmanager.h"

TestManager::TestManager(QObject *parent) : QObject(parent)
{

}

void TestManager::executeAllTests()
{
    listManagerTests();
    mediaLoaderTests();
}

void TestManager::listManagerTests()
{
    ListManagerTests listmanager;
    QTest::qExec(&listmanager);
}

void TestManager::mediaLoaderTests()
{
    MediaLoaderTests mediaLoader;
    QTest::qExec(&mediaLoader);
}
