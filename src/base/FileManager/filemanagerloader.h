#ifndef FILEMANAGERLOADER_H
#define FILEMANAGERLOADER_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDirIterator>
#include <QDir>
#include <QTimer>
#include "src/base/FileManager/mediadirectories.h"
#include "src/base/FileManager/filemanager.h"

class FileManagerLoader : public QObject
{
    Q_OBJECT
public:
    explicit FileManagerLoader(QObject *parent = nullptr);
    static bool loadMediaDirectories(bool mock = false);
    static QString getDirectoriesFileName(bool mock);
    static int getIdFromJson(QJsonObject object);
    static QString getPathFromJson(QJsonObject object);
    static QJsonArray getDirectoriesListArray(QString fileName);

public slots:
    bool searchMediaDirectories();

signals:
    void directoriesLoaded();
};

#endif // FILEMANAGERLOADER_H
