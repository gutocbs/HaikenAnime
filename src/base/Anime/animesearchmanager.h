#ifndef ANIMESEARCHMANAGER_H
#define ANIMESEARCHMANAGER_H

#include <QObject>
#include "src/utilities/Interfaces/ISearchManager.h"
#include "src/base/Anime/animelistmanager.h"

class AnimeSearchManager : public ISearchManager
{

public:
    virtual QVector<anime*> searchMedia(const QString &rnome) = 0;
    int getMediaListIndexFromId(const QString &ridAnime);
    QString buscaIDRapido(const QString &rnomeAnime);
    QString getMediaListNameFromId(const QString &ridAnime);
    QString getMediaEpisodeFromId(const QString &rid);
    QString getMediaScoreFromId(const QString &rid);
    QString getMediaTitleFromId(const QString &rid);
    QString getIdFromMediaTitle(const QString &rid);
    QPointer<anime> fbuscaAnimeNoAno(int, const QString &rid);
    QPointer<anime> getMediaFromId(const QString &rid);
    QPointer<anime> getMediaFromListIndex(const QString &rlista, int posicao);
    QVector<anime*> getMediaListFromId(const QString &id);
    void loadListNames();
    void appendToList(QVector<anime *> &mediaList, Enums::mediaList list, int position);
    QVector<anime *> getMediaList(const QString &listName);

private:
    AnimeListManager animeManager;
};

#endif // ANIMESEARCHMANAGER_H
