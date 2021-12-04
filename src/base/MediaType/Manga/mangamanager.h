#ifndef MANGAMANAGER_H
#define MANGAMANAGER_H

#include <QObject>
#include <QPointer>
#include "src/base/Media/mediamanager.h"
#include "src/utilities/singleton.h"

class MangaManager : public MediaManager
{
    Q_OBJECT
    static MangaManager *createInstance();
    static MangaManager *instance();
public:
    explicit MangaManager(QObject *parent = nullptr);
    IMediaManager *getInstance();
};

#endif // MANGAMANAGER_H
