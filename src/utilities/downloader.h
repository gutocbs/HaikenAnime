#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>
#include <QVector>
#include <QDebug>
#include "src/base/downloaderworker.h"

class Downloader : public QObject
{
    Q_OBJECT
public:
    explicit Downloader(QObject *parent = nullptr);
    ~Downloader();

    void fsetWorker();

signals:
    void sfinishedXML();
    void sfinishedAnimeTorrent();

public slots:
    void setListAndType(QString, QString);
    void setAvatar(QString);
    void work(int value);
    void finished();
    void checkwork();

    void workBig(int value);
    void finishedBig();
    void checkworkBig();


    void workAvatar(int value);
    void finishedAvatar();
    void checkworkAvatar();

    void workXML(int value);
    void finishedXML();
    void checkworkXML();

    void workSpecificXML(int value, QString);
    void finishedSpecificXML();
    void checkworkSpecificXML();

    void workAnimeTorrent(int value, QString, QString);
    void finishedAnimeTorrent();
    void checkworkAnimeTorrent();

    bool isBusy();
private:
    QString searchName;
    QStringList torrentLink;
    QStringList torrentName;
    QVector<DownloaderWorker *> m_workers;
    QVector<int> m_work;
    DownloaderWorker *dw;

};

#endif // DOWNLOADER_H
