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
    void downloadCovers();
    void downloadURLs();
signals:

private:
    static inline QHash<int, DownloadEnums::imageSize> mediaQueue = QHash<int, DownloadEnums::imageSize>();
    static inline QHash<QString, DownloadEnums::imageSize> mediaQueueURL = QHash<QString, DownloadEnums::imageSize>();
    static inline bool isDownloading = false;
};

#endif // DOWNLOADQUEUE_H
