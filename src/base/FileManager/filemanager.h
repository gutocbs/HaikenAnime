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
#include "src/base/FileManager/MediaDirectories.h"

class FileManager : public MediaDirectories
{
    Q_OBJECT
public:
    explicit FileManager(QObject *parent = nullptr);
    static bool getFileExtensions();
    bool compareFileToMediaName(Media *media, QString fileName);
    static bool compareFileToMediaEpisode(Media *media, QString fileName, int episode);
    static bool openFileOrFolder(const QByteArray &path);
    QString searchMediaFolderPath(Media *media);
    QString getMediaEpisodePath(Media *media, int episode = 0);
    QString getMediaFolderPath(Media *media);
    static QString getMediaName(QString fileName);
    static int getMediaEpisode(QString fileName);
signals:
private:
    QStringList fileExtensions;
};

#endif // FILEMANAGER_H
