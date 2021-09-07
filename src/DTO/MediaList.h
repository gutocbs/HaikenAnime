#ifndef MEDIALIST_H
#define MEDIALIST_H

#include <QObject>
#include <QStringList>
#include <QDate> //Pega data de estreia e season
#include "src/utilities/Enums.h"

class MediaList : public QObject
{
    Q_OBJECT
public:
    explicit MediaList(Enums::mediaType type, Enums::mediaList list, int size, QObject *parent = nullptr){
        Q_UNUSED(parent)
        this->type = type;
        this->list = list;
        this->size = size;
    }
    Enums::mediaType type;
    Enums::mediaList list;
    int size;
};


#endif // MEDIALIST_H
