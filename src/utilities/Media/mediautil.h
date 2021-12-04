#ifndef MEDIAUTIL_H
#define MEDIAUTIL_H

#include <QObject>
#include <QFile>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include "src/DTO/media.h"

class MediaUtil : public QObject
{
    Q_OBJECT
public:
    explicit MediaUtil(QObject *parent = nullptr);
    static int getTotalEpisodes(Media *media);
    static int getNumberOfWatchedEpisodes(Media *media);//TODO - LEMBRAR POR QUE PRECISO DISSO
    static QJsonObject getMediaAsJsonObject(Media *media);
signals:

private:
    static QString getMediaNumberReleasedEpisodes(Media *media);
};

#endif // MEDIAUTIL_H
