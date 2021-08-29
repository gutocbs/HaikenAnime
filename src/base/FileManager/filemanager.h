#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include <QPointer>
#include <QFile>
#include <QDirIterator>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include "src/DTO/media.h"
#include "src/utilities/Media/mediacomparer.h"
#include "src/utilities/Media/mediautil.h"
#include "src/base/FileManager/filemanagerloader.h"
#include "src/base/FileManager/filemanagersaver.h"

class FileManager : public QObject
{
    Q_OBJECT
public:
    explicit FileManager(QObject *parent = nullptr);
    bool getFileExtensions();
    bool compareFileToMediaName(Media *media, QString fileName);
    bool compareFileToMediaEpisode(Media *media, QString fileName, int episode);
    bool openFile(const QByteArray &path);
    bool openFolder(const QByteArray &path);
    QString searchMediaFolderPath(Media *media);
    QString getMediaEpisodePath(Media *media, int episode = 0);
    QString getMediaFolderPath(Media *media);
    QString getMediaName(QString fileName);
    int getMediaEpisode(QString fileName);
signals:
private:
    QStringList fileExtensions;
};

#endif // FILEMANAGER_H
