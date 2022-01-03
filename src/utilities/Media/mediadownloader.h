#ifndef MEDIADOWNLOADER_H
#define MEDIADOWNLOADER_H

#include <QObject>
#include <QPointer>
#include <QTimer>
#include "src/utilities/Enums.h"
#include "src/DTO/MediaList.h"
#include "src/utilities/Media/mediacomparer.h"
#include "src/utilities/downloader.h"
//APAGAR?
class MediaDownloader : public QObject
{
    Q_OBJECT
public:
    Q_DECL_DEPRECATED explicit MediaDownloader(QObject *parent = nullptr);
    Q_DECL_DEPRECATED void insertDownloadQueue(QString list, Enums::mediaType mediaType, Enums::mediaList mediaList, int listSize);
    Q_DECL_DEPRECATED void downloadCoverImages();

private:
    QMap<QString, MediaList*> downloadQueue;
    QPointer<Downloader> downloader;
};

#endif // MEDIADOWNLOADER_H
