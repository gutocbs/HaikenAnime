#include "MediaDirectories.h"

MediaDirectories::MediaDirectories(QObject *parent) : QObject(parent)
{

}

bool MediaDirectories::checkIfFileCanBeOpened(QString filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Could not read file!";
        qCritical() << file.errorString();
        return false;
    }
    if(file.size() == 0)
        return false;
    return true;
}

bool MediaDirectories::addMediaDirectory(QString id, QString path)
{
    mediaDirectoriesById.insert(id, path);
    return true;
}
