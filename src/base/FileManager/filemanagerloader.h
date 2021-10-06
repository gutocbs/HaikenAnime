#ifndef FILEMANAGERLOADER_H
#define FILEMANAGERLOADER_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDirIterator>
#include <QDir>
#include "src/base/FileManager/mediadirectories.h"
#include "src/utilities/Media/mediautil.h"
class FileManagerLoader : public MediaDirectories
{
    Q_OBJECT
public:
    explicit FileManagerLoader(QObject *parent = nullptr);
    static bool loadMediaDirectories(bool mock = false);
    static QString getDirectoriesFileName(bool mock);
    static int getIdFromJson(QJsonObject object);
    static QString getPathFromJson(QJsonObject object);
    static QJsonArray getDirectoriesListArray(QString fileName);
signals:
private:
};

#endif // FILEMANAGERLOADER_H
