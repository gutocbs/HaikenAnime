#ifndef MEDIASEARCHMANAGER_H
#define MEDIASEARCHMANAGER_H

#include <QObject>
#include <QPointer>
#include "src/utilities/Interfaces/IMediaSearchManager.h"
#include "src/base/Anime/animelistmanager.h"

class MediaSearchManager : public IMediaSearchManager
{
    Q_OBJECT
public:
    explicit MediaSearchManager(QObject *parent = nullptr, IMediaListManager* mediaListManager = nullptr);
    void setMediaListManager(IMediaListManager *mediaListManager);
    QVector<Media*> searchMedia(const QString &rnome);
    int getMediaListIndexFromId(const QString &ridAnime);
    QString buscaIDRapido(const QString &rnomeAnime);
    QString getMediaListNameFromId(const QString &ridAnime);
    QString getMediaEpisodeFromId(const QString &rid);
    QString getMediaScoreFromId(const QString &rid);
    QString getMediaTitleFromId(const QString &rid);
    QString getIdFromMediaTitle(const QString &rid);
    QPointer<Media> fbuscaAnimeNoAno(int, const QString &rid);
    QPointer<Media> getMediaFromId(const QString &rid);
    QPointer<Media> getMediaFromListIndex(const QString &rlista, int posicao);
    QVector<Media*> getMediaListFromId(const QString &id);
    void loadListNames();
    void appendToList(QVector<Media*> &mediaList, Enums::mediaList list, int position);
    QVector<Media*> getMediaList(const QString &listName);

private:
    QPointer<IMediaListManager> mediaListManager;
};

#endif // MEDIASEARCHMANAGER_H
