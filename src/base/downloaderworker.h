#ifndef DOWNLOADERWORKER_H
#define DOWNLOADERWORKER_H

#include <QObject>
#include <QDebug>
#include <QVector>
#include <QPointer>
#include <QtNetwork>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>

#include "src/base/leitorlistaanimes.h"
#include "src/base/confbase.h"
#include "src/base/abaconfig.h"

class DownloaderWorker : public QObject
{
    Q_OBJECT
public:
    explicit DownloaderWorker(QObject *parent = nullptr);
    bool isBusy();
    void fsetURLAvatar(const QString &urlAvatar);
    void fdownloadTorrent(const QString &fileURL, const QString &torrentName);
    void fdownloadXMLTorrentList();

signals:
    void started();
    void finished();
    void finishedBig();
    void finishedXML();
    void sterminouLista(QString);

public slots:
//    void timeout();
    void fdownloadAvatarUsuario(int value);
    void ffinishedAvatar();
    void ffinishedGeneralXML();
    void work(int value);
    void ffinished();
    void workBig(int value);
    void ffinishedBig();
    void fselecionaLista(QString, QString);

private slots:
    void onFinished(QNetworkReply*);
    void onReadyRead();

private:
    bool m_busy;
    bool vfileIsOpen;

    int vlista;
    int vindexLista;

    QString vlink;
    QString vsaveFilePath;
    QString vURLAvatar;

    QNetworkAccessManager *vmanager;
    QNetworkReply *vreply;
    QFile *vfile;

    QVector<anime*> vlistaSelecionada;
    QPointer<leitorlistaanimes> cleitorlistaanimes;
    QPointer<confBase> cconfBase;
};

#endif // DOWNLOADERWORKER_H
