#ifndef MEDIALISTMANAGER_H
#define MEDIALISTMANAGER_H

#include <QObject>
#include <QVariant>
#include <QPointer>
#include "src/utilities/Enums.h"
#include "src/DTO/ListOrder.h"
class MediaListManager : public QObject
{
    Q_OBJECT
public:
    explicit MediaListManager(QObject *parent = nullptr);
    static void setListOrder(QVariant newOrder);

signals:

};

#endif // MEDIALISTMANAGER_H
