#ifndef MEDIALISTCONTROLLER_H
#define MEDIALISTCONTROLLER_H

#include <QObject>
#include "src/controllers/mediainformationcontroller.h"

class MediaListController : public QObject
{
    Q_OBJECT
public:
    explicit MediaListController(QObject *parent = nullptr);
    void setList(Enums::mediaList newMediaList, int data = 0);
    void selectListCompleted();
    void selectListPaused();
    void selectListDropped();
    void selectListPlanning();
    void setMediaType(Enums::mediaType newMediaType);
    void selectTypeManga();
    void selectTypeNovel();
    void selectListSeason(QVariant data);
    void setMedia();
signals:
private:
    QPointer<MediaInformationController> mediaInformationController;
    QPointer<IMediaListManager> mediaListManager;
    QPointer<IMediaManager> mediaManager;
    QPointer<MediaController> mediaController;
    QPointer<IMediaSearchManager> mediaSearchManager;
};

#endif // MEDIALISTCONTROLLER_H
