#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include <QPointer>
#include <QFile>
#include <QDirIterator>
#include <QDir>
#include "src/DTO/media.h"
#include "src/utilities/Media/mediacomparer.h"
#include "src/utilities/Media/mediautil.h"

class FileManager : public QObject
{
    Q_OBJECT
public:
    explicit FileManager(QObject *parent = nullptr);
    bool getFileExtensions();
    bool compareFileToMedia(Media *media, QString fileName);
    QString searchMediaFolder(Media *media);
    QString getMediaNextEpisodePath(Media *media);
    QString getMediaFolder(Media *media);
    QString getMediaName(QString fileName);
    int getMediaEpisode(QString fileName);
signals:
private:
    QStringList fileExtensions;
};

#endif // FILEMANAGER_H
