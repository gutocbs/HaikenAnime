#ifndef MEDIAINFORMATIONCONTROLLER_H
#define MEDIAINFORMATIONCONTROLLER_H

#include <QObject>
#include <QPointer>
#include <QJsonObject>
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
    int getPageIndexRange();
    void setPage(int numberPages);

    static inline int selectedPage = 0;
    static inline int selectedMediaGridIndex = 0;
    static inline int selectedMediaIndex = 0;
    static inline Enums::mediaList mediaList = Enums::mediaList::CURRENT;
    static inline Enums::mediaType mediaType = Enums::mediaType::ANIME;

signals:
    void signalSelectedMedia(QJsonObject data);
    void signalIdMedia(int listMediaIndex, bool nullSignal = false);
//    void signalIdMedia();

private:
    QPointer<IMediaListManager> mediaListManager;
    QPointer<IMediaManager> mediaManager;
    QPointer<MediaController> mediaController;

};

#endif // MEDIAINFORMATIONCONTROLLER_H
