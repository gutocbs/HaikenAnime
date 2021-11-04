#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <QObject>
#include <QVariant>
#include <QHash>
#include <QHashIterator>
#include "src/clients/ClientEnums.h"
#include "src/utilities/Enums.h"
#include "src/clients/IClient.h"
#include "src/clients/anilist.h"

class ClientManager : public QObject
{
    Q_OBJECT
public:
    explicit ClientManager(QObject *parent = nullptr);
    static bool addToUpdateQueue(ClientEnums::updateType updateType, int mediaId, QVariant updatedValue);
    void setUpdate();
    void downloadMediaList();
signals:
private:
    void updateClient();
    QPointer<IClient> client;
    static inline QHash<int, Enums::mediaList> updateQueueList = QHash<int, Enums::mediaList>();
    static inline QHash<int, int> updateQueueScore = QHash<int, int>();
    static inline QHash<int, int> updateQueueProgress = QHash<int, int>();
};

#endif // CLIENTMANAGER_H
