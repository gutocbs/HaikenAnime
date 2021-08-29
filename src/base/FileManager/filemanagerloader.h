#ifndef FILEMANAGERLOADER_H
#define FILEMANAGERLOADER_H

#include <QObject>

class FileManagerLoader : public QObject
{
    Q_OBJECT
public:
    explicit FileManagerLoader(QObject *parent = nullptr);

signals:

};

#endif // FILEMANAGERLOADER_H
