#ifndef ABATORRENT_H
#define ABATORRENT_H

#include <QObject>
#include <QVector>
#include <QList>
#include <QHash>
#include <QPointer>

#include "src/base/torrentinfo.h"
#include "src/base/abaconfig.h"
#include "src/base/arquivos.h"
#include "src/utilities/downloader.h"
#include "src/base/leitorlistaanimes.h"
#include "lib/anitomy/anitomy.h"

class abaTorrent : public QObject
{
    Q_OBJECT
public:
    explicit abaTorrent(QObject *parent = nullptr);

    //Faz baixar a lista, ler o arquivo XML e montar a tabela
    void fRefreshControl();
    void fgetTorrentList();
    void freadTorrentList();
    void fchangeTorrentState(int);

    int fcheckPriority(torrentinfo*);
    bool fcheckAlreadyDownloaded();

    QList<QVariant> fgetTorrentInfo();

signals:
    void sfimXML();


private:
    anitomy::Anitomy anitomy;
    QPointer<leitorlistaanimes> cleitorListaAnimes;
    QPointer<abaConfig> cabaConfig;
    QVector<torrentinfo*> vlistaTorrents;
    QHash<QString,QString> vHashDeNomeEId;
    QMap<QString, QStringList> vdownloadList;
    QList<QVariant> vinfoTorrent;
};

#endif // ABATORRENT_H
