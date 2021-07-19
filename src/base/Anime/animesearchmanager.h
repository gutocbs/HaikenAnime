#ifndef ANIMESEARCHMANAGER_H
#define ANIMESEARCHMANAGER_H

#include <QObject>
#include <QPointer>
#include "src/utilities/Interfaces/ISearchManager.h"
#include "src/base/Anime/animelistmanager.h"

class AnimeSearchManager : public QObject, public ISearchManager
{
    Q_OBJECT
public:
    explicit AnimeSearchManager(QObject *parent = nullptr);
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
    QPointer<AnimeListManager> animeListManager;
};

#endif // ANIMESEARCHMANAGER_H
