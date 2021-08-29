#ifndef FILEMANAGERSAVER_H
#define FILEMANAGERSAVER_H

#include <QObject>

class FileManagerSaver : public QObject
{
    Q_OBJECT
public:
    explicit FileManagerSaver(QObject *parent = nullptr);

signals:

};

#endif // FILEMANAGERSAVER_H
