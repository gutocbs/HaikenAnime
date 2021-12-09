#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <QObject>
#include <QVariant>
#include <QHash>
#include <QHashIterator>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include "src/clients/ClientEnums.h"
#include "src/utilities/Enums.h"
#include "src/clients/IClient.h"
#include "src/clients/anilist.h"

class ClientManager : public QObject
{
    Q_OBJECT
public:
    explicit ClientManager(QObject *parent = nullptr);
    ~ClientManager();
    static bool addToUpdateQueue(ClientEnums::updateType updateType, int mediaId, QVariant updatedValue);
    void setClient(ClientEnums::clients client = ClientEnums::clients::ANILIST);
    void setAuthCode(const QString &user, QVariant authcode);
    void setUpdate();
    void setConnections();
public slots:
    void downloadMediaList();
    void downloadYearlyLists();
    QString getAvatar();
signals:
    void signalDownloadCompleted(bool);
private slots:
    void sendDownloadSignal(bool signal = true);
    void downloadMediaLists();

private:
    QFutureWatcher<bool> downloadWatcher;
    void updateClient();
    QThread clientThread;
    QPointer<anilist> client;
    static inline QHash<int, Enums::mediaList> updateQueueList = QHash<int, Enums::mediaList>();
    static inline QHash<int, int> updateQueueScore = QHash<int, int>();
    static inline QHash<int, int> updateQueueProgress = QHash<int, int>();
};

#endif // CLIENTMANAGER_H
