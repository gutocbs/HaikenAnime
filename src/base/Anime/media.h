#ifndef ANIME_H
#define ANIME_H

#include <QObject>
#include <QPointer>
#include "src/utilities/Enums.h"
#include "src/base/Anime/animelistmanager.h"
#include "src/base/Anime/mediasearchmanager.h"
#include "src/base/Anime/animemanager.h"
#include "src/utilities/singleton.h"

class Anime : public QObject
{
    Q_OBJECT
    static Anime *createInstance();
public:
    explicit Anime(QObject *parent = nullptr);
    static Anime *instance();
    void initializeMedia();
    QPointer<IMediaListManager> getMediaListManager();
    QPointer<IMediaSearchManager> getMediaSearchManager();
    QPointer<IMediaManager> getMediaManager();

signals:

private:
    QPointer<IMediaListManager> mediaListManager;
    QPointer<IMediaSearchManager> mediaSearchManager;
    QPointer<IMediaManager> mediaManager;
};

#endif // ANIME_H
