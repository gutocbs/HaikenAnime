#ifndef MEDIAOBJECTS_H
#define MEDIAOBJECTS_H

#include <QObject>
#include <QPointer>
#include "src/utilities/Interfaces/IMediaListManager.h"
#include "src/utilities/Interfaces/IMediaManager.h"
#include "src/base/Media/mediacontroller.h"

class MediaObjects : public QObject
{
    Q_OBJECT
public:
    explicit MediaObjects(QObject *parent = nullptr);
    QPointer<IMediaListManager> mediaListManager;
    QPointer<IMediaSearchManager> mediaSearchManager;
    QPointer<IMediaManager> mediaManager;
    QPointer<MediaController> mediaController;
};
#endif // MEDIAOBJECTS_H
