#ifndef MANGALISTMANAGER_H
#define MANGALISTMANAGER_H

#include <QObject>
#include "src/base/Media/medialistmanager.h"
#include "src/utilities/singleton.h"

class MangaListManager : public MediaListManager
{
    Q_OBJECT
    static MangaListManager *createInstance();
    static MangaListManager *instance();
public:
    explicit MangaListManager(QObject *parent = nullptr);
    IMediaListManager *getInstance();
    Enums::mediaType getMediaType();//TODO - OVERWRITE A FUNÇÃO ORIGINAL
};

#endif // MANGALISTMANAGER_H
