#ifndef MEDIACONTROLLER_H
#define MEDIACONTROLLER_H

#include <QObject>
#include <QPointer>
#include "src/utilities/Enums.h"
#include "src/base/Anime/animelistmanager.h"
#include "src/base/Anime/mediasearchmanager.h"
#include "src/base/Anime/animemanager.h"
#include "src/utilities/singleton.h"

//TODO - Essa classe deverá dar load e save nos animes, além de servir como interface para atualizar os animes no anilist
class MediaController : public QObject
{
    Q_OBJECT
    static MediaController *createInstance();
public:
    explicit MediaController(QObject *parent = nullptr);
    static MediaController *instance();
    void initializeMedia();
    void setMediaListManager(Enums::mediaType mediaType);
    void setMediaManager(Enums::mediaType mediaType);
    QPointer<IMediaListManager> getMediaListManager();
    QPointer<IMediaSearchManager> getMediaSearchManager();
    QPointer<IMediaManager> getMediaManager();

signals:

private:
    QPointer<IMediaListManager> mediaListManager;
    QPointer<IMediaSearchManager> mediaSearchManager;
    QPointer<IMediaManager> mediaManager;
};

#endif // MEDIACONTROLLER_H