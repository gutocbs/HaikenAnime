#ifndef MEDIAFILE_H
#define MEDIAFILE_H

#include <QObject>
#include <QPointer>
#include <QFile>
#include <QDirIterator>
#include <QDir>
#include "src/utilities/Media/mediacomparer.h"
#include "src/base/FileManager/filemanager.h"
#include "src/base/Media/mediacontroller.h"

class MediaFile : public QObject
{
    Q_OBJECT
public:
    explicit MediaFile(QObject *parent = nullptr);
    static bool compareFileToMediaName(Media *media, QString fileName);
    static bool compareFileToMediaEpisode(Media *media, QString fileName, int episode);
    static QString getMediaEpisodePath(Media *media, int episode = 0);
    static int getMediaIdFromFile(QString fileName);
    static int getMediaEpisodeFromFile(QString fileName);
    static QString getMediaNameFromFile(QString fileName);
    static QString getSeasonFromFile(QString fileName);
    static QString getNameFromPath(QString path);
    static QString getFolder(QString path);
};

#endif // MEDIAFILE_H
