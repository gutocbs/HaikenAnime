#ifndef MEDIALISTCONTROLLER_H
#define MEDIALISTCONTROLLER_H

#include <QObject>
#include "src/controllers/mediainformationcontroller.h"

class MediaListController : public QObject
{
    Q_OBJECT
public:
    explicit MediaListController(QObject *parent = nullptr);
    void selectListCurrent();
    void selectListCompleted();
    void selectListPaused();
    void selectListDropped();
    void selectListPlanning();
    void selectTypeAnime();
    void selectTypeManga();
    void selectTypeNovel();
    void selectListSeason(QVariant data);
    void setMedia();
    static Enums::mediaList mediaList;
    static Enums::mediaType mediaType;
signals:
private:
    QPointer<MediaInformationController> mediaInformationController;
    QPointer<IMediaListManager> mediaListManager;
    QPointer<IMediaManager> mediaManager;
    QPointer<MediaController> mediaController;
    QPointer<IMediaSearchManager> mediaSearchManager;
};

#endif // MEDIALISTCONTROLLER_H
