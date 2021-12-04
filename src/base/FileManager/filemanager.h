#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include <QPointer>
#include <QFile>
#include <QDirIterator>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include "src/DTO/media.h"

class FileManager : public QObject
{
    Q_OBJECT
public:
    explicit FileManager(QObject *parent = nullptr);
    bool getFileExtensions();
    static bool openFileOrFolder(const QByteArray &path);
    static bool openFileOrFolder(Media *media);
    static bool checkIfFileCanBeOpened(QString filename);
    static inline QStringList fileExtensions;
};

#endif // FILEMANAGER_H
