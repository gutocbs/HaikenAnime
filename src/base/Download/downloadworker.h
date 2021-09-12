#ifndef DOWNLOADWORKER_H
#define DOWNLOADWORKER_H

#include <QObject>
#include <QFile>
#include <QPointer>
#include <QtNetwork>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include "src/utilities/Enums.h"
#include "src/base/Anime/animelistmanager.h"
#include "src/base/Manga/mangalistmanager.h"
#include "src/base/Novel/novellistmanager.h"
#include "src/base/Anime/mediacontroller.h"

class DownloadWorker : public QObject
{
    Q_OBJECT
public:
    enum fileType{Avatar, Cover, Torrent};
    Q_ENUM(fileType)
    enum imageSize{Small, Medium, Big, None = 0};
    Q_ENUM(imageSize)
    explicit DownloadWorker(QObject *parent = nullptr);
    void getList(Enums::mediaList mediaList, Enums::mediaType mediaType);
    void download(fileType fileType, QString url, imageSize imageSize = None);
    void downloadListCovers(Enums::mediaList mediaList, Enums::mediaType mediaType);

signals:
private:
    QString getDownloadPath(fileType fileType, imageSize = None);
    QString getFileName(QString url);
    QByteArray get(QString url);
    qint64 getFileSize(QString url);
    bool saveFile(QPointer<QFile> file, QByteArray fileByteArray);
    void setFinishedSignal(fileType fileType, imageSize = None);
    QPointer<IMediaListManager> listManager;
};

#endif // DOWNLOADWORKER_H
