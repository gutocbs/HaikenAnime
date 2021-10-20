#ifndef ICLIENT_H
#define ICLIENT_H

#include <QObject>
#include "src/utilities/Enums.h"
class IClient : public QObject{
public:
    //TODO - Transformar todas as funções voids em bool ou int
    using QObject::QObject;
    //TODO - Caso a lista seja vazia, deletar da lista
    virtual bool updateList(int mediaId, Enums::mediaList mediaList) = 0;
    virtual bool updateScore(int mediaId, int mediaScore) = 0;
    virtual bool updateProgress(int mediaId, int mediaProgress) = 0;

protected:
};
#endif // ICLIENT_H
