#ifndef FILEMANAGERLOADER_H
#define FILEMANAGERLOADER_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include "src/base/FileManager/mediadirectories.h"
class FileManagerLoader : public MediaDirectories
{
    Q_OBJECT
public:
    explicit FileManagerLoader(QObject *parent = nullptr);
    static QStringList getDirectories();
    static QHash<QString,QString> getMediaDirectories();
    bool loadMediaDirectories(bool mock = false);
    QString getDirectoriesFileName(bool mock);
    QString getIdFromJson(QJsonObject object);
    QString getPathFromJson(QJsonObject object);
    QJsonArray getDirectoriesListArray(QString fileName);
signals:
private:
};

#endif // FILEMANAGERLOADER_H
