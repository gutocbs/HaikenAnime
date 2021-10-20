#include "medialistmanager.h"

MediaListManager::MediaListManager(QObject *parent) : QObject(parent)
{

}

void MediaListManager::setListOrder(QVariant newOrder)
{
    Enums::mediaOrder newOrderEnum = Enums::QStringToMediaOrder(newOrder.toString());
    if(newOrderEnum == ListOrder::getOrder())
        ListOrder::invertOrder();
    else
        ListOrder::setNewOrder(newOrderEnum);
}
