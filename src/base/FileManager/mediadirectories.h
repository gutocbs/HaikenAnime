#ifndef MEDIADIRECTORIES_H
#define MEDIADIRECTORIES_H

#include <QObject>
#include <QHash>
#include <QFile>
#include <QDebug>
#include <QDirIterator>
#include <QDir>
#include <QTimer>
#include "src/base/MediaType/Anime/animemanager.h"
#include "src/utilities/Media/mediautil.h"
#include "src/base/Media/mediafile.h"

class MediaDirectories : public QObject
{
public:
    using QObject::QObject;
    explicit MediaDirectories(QObject *parent = nullptr);
    static bool searchForMediaDirectories();
    static QString getMediaFolderPath(Media *media);
    static QString searchForMediaDirectory(Media *media);
    static bool updateMediaPath(int mediaId, const QString &newDirectoryPath);
    void setSearch();

    static inline QStringList generalDirectories;

    ///QHash<id,Directory>
    static inline QHash<int,QString> mediaDirectoriesById;
private:
};

#endif // MEDIADIRECTORIES_H
