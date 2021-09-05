#ifndef MEDIADIRECTORIES_H
#define MEDIADIRECTORIES_H

#include <QObject>
#include <QHash>
#include <QFile>
#include <QDebug>

class MediaDirectories : public QObject
{
public:
    using QObject::QObject;
    explicit MediaDirectories(QObject *parent = nullptr);
    static bool checkIfFileCanBeOpened(QString filename);
protected:
    bool addMediaDirectory(QString id, QString path);

    QStringList generalDirectories;

    ///QHash<id,Directory>
    QHash<QString,QString> mediaDirectoriesById;
};

#endif // MEDIADIRECTORIES_H
