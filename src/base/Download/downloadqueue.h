#ifndef DOWNLOADQUEUE_H
#define DOWNLOADQUEUE_H

#include <QObject>
#include <QTimer>

#include "src/base/Download/downloadenums.h"
#include "src/base/Download/downloadmanager.h"
#include "src/DTO/media.h"

class DownloadQueue : public QObject
{
    Q_OBJECT
public:
    explicit DownloadQueue(QObject *parent = nullptr);
    static void insertCoverDownloadQueue(int mediaId, DownloadEnums::imageSize imageSize);
    void downloadMedia();
signals:

private:
    static QHash<QVariant, DownloadEnums::imageSize> mediaQueue;

};

#endif // DOWNLOADQUEUE_H
