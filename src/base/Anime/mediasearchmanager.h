#ifndef MEDIASEARCHMANAGER_H
#define MEDIASEARCHMANAGER_H

#include <QObject>
#include <QPointer>
#include "src/utilities/Interfaces/IMediaSearchManager.h"
#include "src/base/Anime/animelistmanager.h"

//TODO - Fazer singleton
class MediaSearchManager : public IMediaSearchManager
{
    Q_OBJECT
public:
    explicit MediaSearchManager(QObject *parent = nullptr);
    void setMediaListManager(IMediaListManager *mediaListManager);
    QVector<Media*> searchMedia(const QString &rnome);
    int buscaIDRapido(const QString &rnomeAnime);
    QPointer<Media> fbuscaAnimeNoAno(int, int idAnime);
    QPointer<Media> getMediaFromId(int rid);
    QVector<Media*> getMediaListFromId(int id);
    void loadListNames();
    void appendToList(QVector<Media*> &mediaList, Enums::mediaList list, int position);
    QVector<Media*> getMediaList(const QString &listName);
    QPointer<Media> getMediaFromListIndex(Enums::mediaList list, int posicao);
    QStringList getNamesById(int id);

    int getMediaEpisodeFromId(int idMedia);
    QString getMediaScoreFromId(int id);
    QString getMediaTitleFromId(int id);
    int getIdFromMediaTitle(const QString &mediaTitle);
    QPointer<Media> fbuscaMediaNoAno(int ano, int id);
private:
//    QPointer<IMediaListManager> mediaListManager;

    // IMediaSearchManager interface
};

#endif // MEDIASEARCHMANAGER_H
