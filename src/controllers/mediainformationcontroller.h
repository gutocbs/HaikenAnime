#ifndef MEDIAINFORMATIONCONTROLLER_H
#define MEDIAINFORMATIONCONTROLLER_H

#include <QObject>
#include <QPointer>
#include "src/utilities/singleton.h"
#include "src/utilities/Enums.h"
#include "src/base/Media/mediacontroller.h"


class MediaInformationController : public QObject
{
    Q_OBJECT
    static MediaInformationController *createInstance();
public:
    explicit MediaInformationController(QObject *parent = nullptr);
    static MediaInformationController *instance();
    void getMediaListPage();
    QVariant getMediaJsonObjectByGridIndex(QVariant gridIndex);
    void getSelectedMediaData(QVariant selectedMediaGridIndex);
    void getMediaList(QVariant order = "Title", QVariant year = 0, bool changeOrder = false);
    static int selectedPage;
    static int selectedMediaGridIndex;
    static int selectedMediaIndex;
    static Enums::mediaList mediaList;

signals:
    void signalSelectedMedia(QVariant data);
    void signalSelectedMediaCover(QVariant data);
    void signalIdMedia(int listMediaIndex, bool nullSignal = false);

private:
    int getPageIndexRange();
    QPointer<IMediaListManager> mediaListManager;
    QPointer<IMediaManager> mediaManager;
    QPointer<MediaController> mediaController;

};

#endif // MEDIAINFORMATIONCONTROLLER_H
