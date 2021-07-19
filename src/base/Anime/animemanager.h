#ifndef ANIMEMANAGER_H
#define ANIMEMANAGER_H

#include <QObject>
#include <QPointer>
#include "src/utilities/Interfaces/IMediaManager.h"
#include "src/base/Anime/animesearchmanager.h"
#include "src/base/Anime/animelistmanager.h"

class AnimeManager : public QObject, public IMediaManager
{
public:
    explicit AnimeManager(QObject *parent = nullptr);
    bool updateMediaList(const QString &mediaId, Enums::mediaList newList);
    bool updateScore(const QString &mediaId, const QString &newScore);
    bool updateProgress(const QString &mediaId, const QString &mediaProgress);
    bool deleteFromList(const QString &mediaId);
    bool insertCustomName(const QString &mediaId, const QStringList &mediaTitle);


private:
    QPointer<AnimeSearchManager> animeSearchManager;
    QPointer<AnimeListManager> animeListManager;
};

#endif // ANIMEMANAGER_H
