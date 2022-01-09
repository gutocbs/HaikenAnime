#ifndef MEDIADATACONTROLLER_H
#define MEDIADATACONTROLLER_H

#include <QObject>
#include <QVariant>
#include <QDesktopServices>

#include "src/controllers/mediainformationcontroller.h"
#include "src/clients/clientmanager.h"
class MediaDataController : public QObject
{
    Q_OBJECT
public:
    explicit MediaDataController(QObject *parent = nullptr);
    static void playNextEpisode();
    void openMediaWebpage(QVariant data);
    void openMediaFolder();
    void setMediaProgress(int mediaId, int mediaProgress);
    void setMediaCustomName(QVariant data);
    void removeMediaFromList();
    void setMediaList(QVariant data);
signals:
private:
    QPointer<IMediaListManager> mediaListManager;
    QPointer<IMediaManager> mediaManager;
    QPointer<MediaController> mediaController;
    QPointer<ClientManager> clientManager;
    QPointer<MediaInformationController> mediaInformationController;
};

#endif // MEDIADATACONTROLLER_H
