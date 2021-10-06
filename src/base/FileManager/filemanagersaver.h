#ifndef FILEMANAGERSAVER_H
#define FILEMANAGERSAVER_H

#include <QObject>
#include "src/base/FileManager/mediadirectories.h"

class FileManagerSaver : public MediaDirectories
{
    Q_OBJECT
public:
    explicit FileManagerSaver(QObject *parent = nullptr);

signals:

};

#endif // FILEMANAGERSAVER_H
