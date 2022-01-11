#ifndef LISTORDER_H
#define LISTORDER_H

#include <QObject>
#include "src/utilities/Enums.h"

class ListOrder : public QObject
{
    Q_OBJECT
public:
    explicit ListOrder(QObject *parent = nullptr){
        Q_UNUSED(parent)
    }

    static void setNewOrder(Enums::mediaOrder newOrder){
        order = newOrder;
        orderType = Enums::orderType::Ascending;
    }

    static void invertOrder(){
        if(orderType == Enums::orderType::Ascending)
            orderType = Enums::orderType::Descending;
        else
            orderType = Enums::orderType::Ascending;
    }

    static Enums::mediaOrder getOrder(){
        return order;
    }

    static Enums::orderType getOrderType(){
        return orderType;
    }

private:
    static inline Enums::mediaOrder order = Enums::mediaOrder::StartDate;
    static inline Enums::orderType orderType = Enums::orderType::Ascending;
};

#endif // LISTORDER_H
