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
public:
    explicit AnimeManager(QObject *parent = nullptr);
    void setMediaSearchManager(IMediaSearchManager *mediaSearchManager);
    void setMediaListManager(IMediaListManager *mediaListManager);

    bool updateMediaList(const QString &mediaId, Enums::mediaList newList);
    bool updateScore(const QString &mediaId, const QString &newScore);
    bool updateProgress(const QString &mediaId, const QString &mediaProgress);
    bool deleteFromList(const QString &mediaId);
    bool insertCustomName(const QString &mediaId, const QStringList &mediaTitle);


private:
    QPointer<IMediaSearchManager> mediaSearchManager;
    QPointer<IMediaListManager> mediaListManager;
};

#endif // ANIMEMANAGER_H
