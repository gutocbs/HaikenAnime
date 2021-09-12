#ifndef MANGAMANAGER_H
#define MANGAMANAGER_H

#include <QObject>
#include <QPointer>
#include "src/utilities/Interfaces/IMediaManager.h"
#include "src/utilities/Interfaces/IMediaSearchManager.h"
#include "src/base/Anime/mediasearchmanager.h"
#include "src/base/Anime/animelistmanager.h"

class MangaManager : public IMediaManager
{
    Q_OBJECT
    static MangaManager *createInstance();
public:
    explicit MangaManager(QObject *parent = nullptr);
    static MangaManager *instance();
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

#endif // MANGAMANAGER_H
