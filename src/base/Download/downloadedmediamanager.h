#ifndef DOWNLOADEDMEDIAMANAGER_H
#define DOWNLOADEDMEDIAMANAGER_H

#include <QObject>
#include "src/base/Download/downloadenums.h"
#include "src/DTO/media.h"

class DownloadedMediaManager : public QObject
{
    Q_OBJECT
public:
    explicit DownloadedMediaManager(QObject *parent = nullptr);
    static bool mediaExists(const int &mediaId);
    static void readDownloadedMediaFiles();

private:
    static inline QHash<int, bool> downloadedMedia = QHash<int, bool>();

};

#endif // DOWNLOADEDMEDIAMANAGER_H
