#ifndef NOVELMANAGER_H
#define NOVELMANAGER_H

#include <QObject>
#include <QPointer>
#include "src/utilities/Interfaces/IMediaManager.h"
#include "src/utilities/Interfaces/IMediaSearchManager.h"
#include "src/base/Anime/mediasearchmanager.h"
#include "src/base/Anime/animelistmanager.h"

class NovelManager : public IMediaManager
{
    Q_OBJECT
public:
    static NovelManager *createInstance();
public:
    explicit NovelManager(QObject *parent = nullptr);
    static NovelManager *instance();
    void setMediaSearchManager(IMediaSearchManager *mediaSearchManager);
    void setMediaListManager(IMediaListManager *mediaListManager);

    bool updateMediaList(int id, Enums::mediaList newList);
    bool updateScore(int id, const QString &newScore);
    bool updateProgress(int id, int mediaProgress);
    bool deleteFromList(int id);
    bool insertCustomName(int id, const QStringList &mediaTitle);
    bool updatePath(int id, const QString &mediaPath);


private:
    QPointer<IMediaSearchManager> mediaSearchManager;
    QPointer<IMediaListManager> mediaListManager;
};

#endif // NOVELMANAGER_H
