#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>
#include <QVariant>
#include "src/base/Download/downloadworker.h"
#include "src/utilities/singleton.h"

class DownloadManager : public QObject
{
    Q_OBJECT
    static DownloadManager *createInstance();
public:
    static DownloadManager *instance();
    explicit DownloadManager(QObject *parent = nullptr);
    void setWorker();
    int getWorkersNumber();

signals:

public slots:
    void setWork(QVariant value);
    void work();
//    void work(int value);
    void work(QString value);

private slots:
    void checkwork();

private:
    void setDownload(DownloadWorker* dw);
    QPointer<Media> getMedia(int id);
    QString searchName;
    QStringList torrentLink;
    QStringList torrentName;
    QVector<DownloadWorker*> workers;
    QVector<QVariant> workVector;

};

#endif // DOWNLOADMANAGER_H
