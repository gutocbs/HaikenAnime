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
#include <QFileInfo>

#include "src/base/database.h"
#include "src/base/confbase.h"
#include "src/base/abaconfig.h"


#include "src/utilities/Enums.h"
#include "src/base/MediaType/Anime/animelistmanager.h"

class DownloaderWorker : public QObject
{
    Q_OBJECT
public:
    explicit DownloaderWorker(QObject *parent = nullptr);
    ~DownloaderWorker();
    bool isBusy();
    void fsetURLAvatar(const QString &urlAvatar);
    void fdownloadTorrent(const QString &fileURL, const QString &torrentName);
    void fdownloadXMLTorrentList();
    void fdownloadSpecificXMLTorrentList(QString);
    void fdownloadAnimeTorrent(QString, QString, int);

signals:
    void started();
    void finished();
    void finishedBig();
    void finishedXML();
    void finishedSpecificXML();
    void finishedAnimeTorrent();
    void sterminouLista(QString);

public slots:
//    void timeout();
    void fdownloadAvatarUsuario(int value);
    void ffinishedAvatar();
    void ffinishedGeneralXML();
    void ffinishedSpecificXML();
    void ffinishedAnimeTorrent();
    void work(int value);
    void ffinished();
    void workBig(int value);
    void ffinishedBig();
    Q_DECL_DEPRECATED void fselecionaLista(QString, QString);

private slots:
    void onFinished(QNetworkReply*);
    void onReadyRead();

private:
    bool m_busy;
    bool vfileIsOpen;

//    int vlista;
    int vindexLista;

    QString vlista;
    QString vlink;
    QString vsaveFilePath;
    QString vURLAvatar;

    QNetworkAccessManager *vmanager;
    QNetworkReply *vreply;
    QFile *vfile;

    QVector<anime*> vlistaSelecionada;
    QPointer<anime> animeSelecionado;
    QPointer<Database> cdatabase;
    QPointer<confBase> cconfBase;
    QPointer<IMediaListManager> listManager;
};

#endif // DOWNLOADERWORKER_H
