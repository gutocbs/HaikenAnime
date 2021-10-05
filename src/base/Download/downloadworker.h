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
#include "src/base/Download/downloadenums.h"
#include "src/base/Anime/mediacontroller.h"

class DownloadWorker : public QObject
{
    Q_OBJECT
public:
    explicit DownloadWorker(QObject *parent = nullptr);
    void download(DownloadEnums::fileType fileType, QString url, DownloadEnums::imageSize imageSize = DownloadEnums::imageSize::None);
    bool isBusy();

signals:
private:
    QString getDownloadPath(DownloadEnums::fileType fileType, DownloadEnums::imageSize = DownloadEnums::imageSize::None);
    QString getFileName(QString url);
    QByteArray get(QString url);
    qint64 getFileSize(QString url);
    bool saveFile(QPointer<QFile> file, QByteArray fileByteArray);
    void setFinishedSignal(DownloadEnums::fileType fileType, DownloadEnums::imageSize = DownloadEnums::imageSize::None);
    bool isBusyDownloading;
};

#endif // DOWNLOADWORKER_H
