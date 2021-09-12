#ifndef MEDIACONTROLLER_H
#define MEDIACONTROLLER_H

#include <QObject>
#include <QPointer>
#include "src/utilities/Enums.h"
#include "src/base/Anime/animelistmanager.h"
#include "src/base/Manga/mangalistmanager.h"
#include "src/base/Novel/novellistmanager.h"
#include "src/base/Anime/mediasearchmanager.h"
#include "src/base/Anime/animemanager.h"
#include "src/base/Manga/mangamanager.h"
#include "src/base/Novel/novelmanager.h"
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
    QPointer<IMediaListManager> getMediaListManager(Enums::mediaType mediaType = Enums::mediaType::NONE);
    QPointer<IMediaManager> getMediaManager(Enums::mediaType mediaType = Enums::mediaType::NONE);
    QPointer<IMediaSearchManager> getMediaSearchManager();

signals:

private:
    QPointer<AnimeListManager> animeListManager;
    QPointer<MangaListManager> mangaListManager;
    QPointer<NovelListManager> novelListManager;
    QPointer<AnimeManager> animeManager;
    QPointer<MangaManager> mangaManager;
    QPointer<NovelManager> novelManager;
    QPointer<IMediaListManager> mediaListManager;
    QPointer<IMediaSearchManager> mediaSearchManager;
    QPointer<IMediaManager> mediaManager;
};

#endif // MEDIACONTROLLER_H
