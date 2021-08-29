#ifndef MEDIALOADERTESTS_H
#define MEDIALOADERTESTS_H

#include <QObject>
#include <QPointer>
#include <QTest>
#include "src/base/Anime/medialoader.h"

class MediaLoaderTests : public QObject
{
    Q_OBJECT
public:
    explicit MediaLoaderTests(QObject *parent = nullptr);

signals:
private:
    QPointer<MediaLoader> mediaLoader;
    QPointer<AnimeListManager> animeListManager;
private slots:
    void initTestCase();
    void cleanupTestCase();
    void loadMediaFromFile_WhenCalled_returnTrue();
    void loadMediaFromFile_WhenCalled_returnFalse();
};

#endif // MEDIALOADERTESTS_H
