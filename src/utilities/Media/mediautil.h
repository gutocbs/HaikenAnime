#ifndef MEDIAUTIL_H
#define MEDIAUTIL_H

#include <QObject>
#include <QFile>
#include <QJsonObject>
#include <QFile>
#include "src/DTO/media.h"
#include "src/base/Anime/mediasearchmanager.h"
#include "src/base/Anime/animelistmanager.h"
#include "src/base/FileManager/filemanager.h"

class MediaUtil : public QObject
{
    Q_OBJECT
public:
    //TODO - funções
    explicit MediaUtil(QObject *parent = nullptr);
    static int getTotalEpisodes(Media *media);
    static int getNumberOfWatchedEpisodes(Media *media);
    static int getMediaIdFromFile(QString fileName);
    static int getMediaEpisodeFromFile(QString fileName);
    static QString getMediaNameFromFile(QString fileName);
    static QString getSeasonFromFile(QString fileName);
    static bool checkIfFileCanBeOpened(QString filename);
    static QJsonObject getMediaAsJsonObject(Media *media);
    static bool openMediaFileOrFolder(Media *media);
signals:

private:
    static QString getMediaNumberReleasedEpisodes(Media *media);
};

#endif // MEDIAUTIL_H
