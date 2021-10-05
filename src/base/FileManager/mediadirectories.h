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
    static bool addMediaDirectory(QString id, QString path);

    static QStringList generalDirectories;

    ///QHash<id,Directory>
    static QHash<QString,QString> mediaDirectoriesById;
    static QStringList fileExtensions;
};

#endif // MEDIADIRECTORIES_H
