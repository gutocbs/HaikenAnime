#ifndef ANIMEMANAGER_H
#define ANIMEMANAGER_H

#include <QObject>
#include <QPointer>
#include "src/utilities/Interfaces/IMediaManager.h"
#include "src/utilities/Interfaces/IMediaSearchManager.h"
#include "src/base/Anime/mediasearchmanager.h"
#include "src/base/Anime/animelistmanager.h"

class AnimeManager : public IMediaManager
{
    Q_OBJECT
    static AnimeManager *createInstance();
public:
    explicit AnimeManager(QObject *parent = nullptr);
    IMediaManager *getInstance();
    static AnimeManager *instance();
    void setMediaSearchManager(IMediaSearchManager *mediaSearchManager);
    void setMediaListManager(IMediaListManager *mediaListManager);

    bool updateMediaList(int id, Enums::mediaList newList);
    bool updateScore(int id, const QString &newScore);
    bool updateProgress(int id, int mediaProgress);
    bool deleteFromList(int id);
    bool insertCustomName(int id, const QStringList &mediaTitle);
    //TODO - Criar função
    bool updatePath(int id, const QString &mediaPath);


private:
    QPointer<IMediaSearchManager> mediaSearchManager;
    QPointer<IMediaListManager> mediaListManager;

    // IMediaManager interface
public:
};

#endif // ANIMEMANAGER_H
