#ifndef FILEMANAGERLOADER_H
#define FILEMANAGERLOADER_H

#include <QObject>

class FileManagerLoader : public QObject
{
    Q_OBJECT
public:
    explicit FileManagerLoader(QObject *parent = nullptr);
    static QStringList getMediaDirectories();
    bool loadMediaDirectories();
signals:
private:
    QStringList mediaDirectories;
};

#endif // FILEMANAGERLOADER_H
