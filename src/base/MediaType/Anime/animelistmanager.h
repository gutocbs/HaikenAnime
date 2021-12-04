#ifndef ANIMELISTMANAGER_H
#define ANIMELISTMANAGER_H

#include <QObject>
#include "src/base/Media/medialistmanager.h"
#include "src/utilities/singleton.h"

class AnimeListManager : public MediaListManager
{
    Q_OBJECT
    static AnimeListManager *createInstance();
    static AnimeListManager *instance();
public:
    explicit AnimeListManager(QObject *parent = nullptr);
    IMediaListManager *getInstance();
    Enums::mediaType getMediaType();//TODO - OVERWRITE A FUNÇÃO ORIGINAL
};

#endif // ANIMELISTMANAGER_H
