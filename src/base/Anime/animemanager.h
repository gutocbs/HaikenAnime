#ifndef ANIMEMANAGER_H
#define ANIMEMANAGER_H

#include <QObject>
#include <QPointer>
#include "src/utilities/Interfaces/IMediaManager.h"
#include "src/utilities/Interfaces/IMediaSearchManager.h"
#include "src/base/Anime/mediasearchmanager.h"
#include "src/base/Anime/animelistmanager.h"

//TODO - Fazer singleton
class AnimeManager : public IMediaManager
{
public:
    explicit AnimeManager(QObject *parent = nullptr);
    void setMediaSearchManager(IMediaSearchManager *mediaSearchManager);
    void setMediaListManager(IMediaListManager *mediaListManager);

    bool updateMediaList(int id, Enums::mediaList newList);
    bool updateScore(int id, const QString &newScore);
    bool updateProgress(int id, int mediaProgress);
    bool deleteFromList(int id);
    bool insertCustomName(int id, const QStringList &mediaTitle);


private:
    QPointer<IMediaSearchManager> mediaSearchManager;
    QPointer<IMediaListManager> mediaListManager;
};

#endif // ANIMEMANAGER_H
