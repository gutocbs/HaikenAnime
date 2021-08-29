#ifndef FILEMANAGERLOADER_H
#define FILEMANAGERLOADER_H

#include <QObject>
#include "src/base/FileManager/mediadirectories.h"

class FileManagerLoader : public MediaDirectories
{
    Q_OBJECT
public:
    explicit FileManagerLoader();
    static QStringList getDirectories();
    static QHash<QString,QString> getMediaDirectories();
    bool loadMediaDirectories();
signals:
private:
};

#endif // FILEMANAGERLOADER_H
