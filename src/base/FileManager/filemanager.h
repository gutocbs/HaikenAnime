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
#include "src/base/FileManager/mediadirectories.h"

class FileManager : public MediaDirectories
{
    Q_OBJECT
public:
    explicit FileManager(QObject *parent = nullptr);
    bool getFileExtensions();
    static bool compareFileToMediaName(Media *media, QString fileName);
    static bool compareFileToMediaEpisode(Media *media, QString fileName, int episode);
    static bool openFileOrFolder(const QByteArray &path);
    static QString searchForMediaDirectory(Media *media);
    static QString getMediaEpisodePath(Media *media, int episode = 0);
    static QString getMediaFolderPath(Media *media);
signals:
private:
};

#endif // FILEMANAGER_H
