#ifndef MEDIADOWNLOADER_H
#define MEDIADOWNLOADER_H

#include <QObject>
#include <QPointer>
#include <QTimer>
#include "src/utilities/Enums.h"
#include "src/DTO/MediaList.h"
#include "src/utilities/Media/mediacomparer.h"
#include "src/utilities/downloader.h"

class MediaDownloader : public QObject
{
    Q_OBJECT
public:
    explicit MediaDownloader(QObject *parent = nullptr);
    void insertDownloadQueue(QString list, Enums::mediaType mediaType, Enums::mediaList mediaList, int listSize);
    void downloadCoverImages();

private:
    QMap<QString, MediaList*> downloadQueue;
    QPointer<Downloader> downloader;
};

#endif // MEDIADOWNLOADER_H
