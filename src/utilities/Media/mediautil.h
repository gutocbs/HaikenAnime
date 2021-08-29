#ifndef MEDIAUTIL_H
#define MEDIAUTIL_H

#include <QObject>
#include "src/DTO/media.h"

class MediaUtil : public QObject
{
    Q_OBJECT
public:
    //TODO - funções
    explicit MediaUtil(QObject *parent = nullptr);
    static int getTotalEpisodes(Media *media);
    static int getNumberOfWatchedEpisodes(Media *media);
signals:

};

#endif // MEDIAUTIL_H
