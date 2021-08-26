#ifndef LISTMANAGERTESTS_H
#define LISTMANAGERTESTS_H

#include <QObject>
#include <QPointer>
#include <QTest>
#include "src/base/Anime/animelistmanager.h"
#include "src/base/Anime/medialoader.h"

class ListManagerTests : public QObject
{
    Q_OBJECT
public:
    explicit ListManagerTests(QObject *parent = nullptr);

signals:

private:
    QPointer<AnimeListManager> animeListManager;
    QPointer<MediaLoader> mediaLoader;
private slots:
    void initTestCase();
    void cleanupTestCase();
    void getMediaList_WhenCalled_ReturnFullMediaList();
    void getHashMediaIndexById_WhenCalled_ReturnFullhashMediaIndexById();
    void getHashMediaListById_WhenCalled_ReturnFullhashMediaListById();
    void getHashMediaNamesById_WhenCalled_ReturnFullhashMediaNamesById();
};

#endif // LISTMANAGERTESTS_H
