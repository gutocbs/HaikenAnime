#ifndef ANIMESEARCHMANAGER_H
#define ANIMESEARCHMANAGER_H

#include <QObject>
#include "src/utilities/Interfaces/ISearchManager.h"
#include "src/base/Anime/animemanager.h"

class AnimeSearchManager : public ISearchManager
{
public:

signals:


    // ISearchManager interface
public:
    int getListIndexFromId(const QString &ridAnime);
    QString buscaIDRapido(const QString &rnomeAnime);
    QString getListFromId(const QString &ridAnime);
    QString getMediaEpisodeFromId(const QString &rid);
    QString getMediaScoreFromId(const QString &rid);
    QString getMediaTitleFromId(const QString &rid);
    QString getIdFromMediaTitle(const QString &rid);
    QPointer<anime> fbuscaAnimeNoAno(int, const QString &rid);
    QPointer<anime> getMediaFromId(const QString &rid);
    QPointer<anime> getMediaFromListIndex(const QString &rlista, int posicao);
    QVector<anime*> getMediaListFromId(const QString &id);
    void loadListNames();

private:
    AnimeManager animeManager;

};

#endif // ANIMESEARCHMANAGER_H
