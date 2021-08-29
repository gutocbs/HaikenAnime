#ifndef MEDIADIRECTORIES_H
#define MEDIADIRECTORIES_H

#include <QObject>
#include <QHash>

class MediaDirectories : public QObject
{
public:
    using QObject::QObject;
protected:
    QStringList generalDirectories;
    QHash<QString,QString> mediaDirectoriesById;
};

#endif // MEDIADIRECTORIES_H
