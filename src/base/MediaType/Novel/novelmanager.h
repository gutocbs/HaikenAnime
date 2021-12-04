#ifndef NOVELMANAGER_H
#define NOVELMANAGER_H

#include <QObject>
#include <QPointer>
#include "src/base/Media/mediamanager.h"
#include "src/utilities/singleton.h"

class NovelManager : public MediaManager
{
    Q_OBJECT
    static NovelManager *createInstance();
    static NovelManager *instance();
public:
    explicit NovelManager(QObject *parent = nullptr);
    IMediaManager *getInstance();
};

#endif // NOVELMANAGER_H
