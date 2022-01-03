#ifndef FILEMANAGERSAVER_H
#define FILEMANAGERSAVER_H

#include <QObject>
#include <QHashIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "src/base/FileManager/mediadirectories.h"

class FileManagerSaver : public MediaDirectories
{
    Q_OBJECT
public:
    explicit FileManagerSaver(QObject *parent = nullptr);
    static bool saveMediaDirectories();

signals:
private:
    static QJsonDocument getMediaDirectoriesJsonDocument();
    static bool removeAndRenameTempSaveFile();
};

#endif // FILEMANAGERSAVER_H
