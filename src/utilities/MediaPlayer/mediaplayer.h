#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QObject>
#include <QPointer>
#include <QScopedPointer>
#include <vector>
#include "lib/robotlib/Robot.h"
#include "src/DTO/media.h"
#include "src/DTO/CurrentMediaPlaying.h"
#include "src/base/Media/mediacontroller.h"

class MediaPlayer : public QObject
{
    Q_OBJECT
public:
    explicit MediaPlayer(QObject *parent = nullptr);
    QPointer<CurrentMediaPlaying> getCurrentMediaPlaying();

signals:
private:
    static QString getCleanWindowName(std::string windowName);
    static QString getMediaName(QString windowName);
    static int getEpisode(QString windowName);
    static bool compareWindowNameToMediaPlayer(QString windowName);
    QPointer<IMediaSearchManager> mediaSearchManager;
};

#endif // MEDIAPLAYER_H
