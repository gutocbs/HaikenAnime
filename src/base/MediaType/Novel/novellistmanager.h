#ifndef NOVELLISTMANAGER_H
#define NOVELLISTMANAGER_H

#include <QObject>
#include "src/base/Media/medialistmanager.h"
#include "src/utilities/singleton.h"

class NovelListManager : public MediaListManager
{
    Q_OBJECT
    static NovelListManager *createInstance();
    static NovelListManager *instance();
public:
    explicit NovelListManager(QObject *parent = nullptr);
    IMediaListManager *getInstance();
    Enums::mediaType getMediaType();//TODO - OVERWRITE A FUNÇÃO ORIGINAL
};

#endif // NOVELLISTMANAGER_H
