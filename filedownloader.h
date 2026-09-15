#ifndef FILEDOWNLOADER_H
#define FILEDOWNLOADER_H

#include <QObject>
#include <QtNetwork>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPointer>
#include <QDate>//Checar qual ano estamos

#include "leitorlistaanimes.h"
#include "confbase.h"



class filedownloader : public QObject
{
    Q_OBJECT
public:
    explicit filedownloader(QObject *parent = nullptr);
    ~filedownloader();
    void fsetLeitorListaAnimes(leitorlistaanimes*);
    void fsetConfBase(confBase*);
    void fdownloadAvatarUsuario(const QString &fileURL);
    void fdownloadTorrent(const QString &fileURL, const QString &torrentName);
    void fdownloadXMLTorrentList(const QString &fileURL);

signals:
    void slistaMensagem(QString);
    void savatar();
    void sxml();
    void storrent();
    void sid(QString);
    void sidGrande(QString);
    void sterminouLista(QString);

public slots:
    void fsetDownload(bool);
    void fdownloadMedio();
    void fdownloadGrande();
    void fdownloadPequeno();
    void fsetNext();
    void fsetNextBig();
    void fsetNextSmall();
    void fresetCounters();
    void fterminouAvatar();
    void fdownloadPorAno(int);
    void fsetNextAno();

private slots:
    void onFinished(QNetworkReply*);
    void onReadyRead();

private:
    QNetworkAccessManager *vmanager;
    QNetworkReply *vreply;
    QFile *vfile;
    QPointer<confBase> cconfBase;
    QPointer<leitorlistaanimes> cleitorlistaanimes;
    QVector<anime*> vlistaSelecionada;

    QString vlistaAtual;//Lista que está sendo lida
    QString vsaveFilePath;
    QString vlink;

    bool vdownloadPequeno  = false;
    bool vfileIsOpen;
    bool vterminouLista;
    bool vdownloadAcontecendo;
    bool vdownloadAnoAcontecendo;
    int vlista;
    int vindexLista;
    int ano;
};

#endif // FILEDOWNLOADER_H
