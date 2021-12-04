#ifndef ANIMEMANAGER_H
#define ANIMEMANAGER_H

#include <QObject>
#include <QPointer>
#include "src/base/Media/mediamanager.h"
#include "src/utilities/singleton.h"

class AnimeManager : public MediaManager
{
    Q_OBJECT
    static AnimeManager *createInstance();
    static AnimeManager *instance();
public:
    explicit AnimeManager(QObject *parent = nullptr);
    IMediaManager *getInstance();
};

#endif // ANIMEMANAGER_H
