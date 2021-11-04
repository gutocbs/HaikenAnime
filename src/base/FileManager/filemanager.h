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
class FileManager : public QObject
{
    Q_OBJECT
public:
    explicit FileManager(QObject *parent = nullptr);
    bool getFileExtensions();
    static bool compareFileToMediaName(Media *media, QString fileName);
    static bool compareFileToMediaEpisode(Media *media, QString fileName, int episode);
    static bool openFileOrFolder(const QByteArray &path);
    static QString getMediaEpisodePath(Media *media, int episode = 0);
    static QString getMediaFolderPath(Media *media);
    static inline QStringList fileExtensions;
};

#endif // FILEMANAGER_H
