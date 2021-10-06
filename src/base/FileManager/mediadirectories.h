#ifndef MEDIADIRECTORIES_H
#define MEDIADIRECTORIES_H

#include <QObject>
#include <QHash>
#include <QFile>
#include <QDebug>
#include <QDirIterator>
#include <QDir>
#include "src/base/Anime/mediasearchmanager.h"
#include "src/base/Anime/animelistmanager.h"
#include "src/base/Anime/animemanager.h"
#include "src/utilities/Media/mediautil.h"

class MediaDirectories : public QObject
{
public:
    using QObject::QObject;
    explicit MediaDirectories(QObject *parent = nullptr);
protected:
    static bool addMediaDirectory(int id, QString path);
    static bool searchForMediaDirectories();

    static inline QStringList generalDirectories;

    ///QHash<id,Directory>
    static inline QHash<int,QString> mediaDirectoriesById;
    static inline QStringList fileExtensions;

private:
    static inline QVector<int> getMediaPathsToSearch();
    static inline bool updateMediaPath(QVector<int> mediaToSearch, QString path);
};

#endif // MEDIADIRECTORIES_H
