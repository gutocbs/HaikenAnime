#ifndef CURRENTMEDIAPLAYING_H
#define CURRENTMEDIAPLAYING_H
#include <QObject>
#include <QStringList>
#include <QDate> //Pega data de estreia e season
#include "src/utilities/Enums.h"

class CurrentMediaPlaying : public QObject
{
    Q_OBJECT
public:
    explicit CurrentMediaPlaying(int mediaId, QString mediaName, int mediaEpisode, QObject *parent = nullptr){
        Q_UNUSED(parent)
        this->mediaId = mediaId;
        this->mediaEpisode = mediaEpisode;
        this->mediaName = mediaName;
    }
    int mediaId;
    int mediaEpisode;
    QString mediaName;
};


#endif // CURRENTMEDIAPLAYING_H
