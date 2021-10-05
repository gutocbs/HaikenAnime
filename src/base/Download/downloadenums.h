#ifndef DOWNLOADENUMS_H
#define DOWNLOADENUMS_H

#include <QObject>

class DownloadEnums : public QObject
{
    Q_OBJECT
public:
    enum fileType{Avatar, Cover, Torrent};
    Q_ENUM(fileType)
    enum imageSize{Small, Medium, Big, None = 0};
    Q_ENUM(imageSize)
};


#endif // DOWNLOADENUMS_H
