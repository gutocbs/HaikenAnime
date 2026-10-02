#ifndef IMEDIAMANAGER_H
#define IMEDIAMANAGER_H

#include <QObject>
#include "src/utilities/Enums.h"
#include "src/base/anime.h"
#include "src/utilities/singleton.h"

class IMediaManager : QObject{
    Q_OBJECT
public:

    virtual QHash<QString, int> getHash(Enums::hashList, int valorInt = 0) = 0;
    bool fmudaLista(const QString &rid, const QString &rlista, type rtipo);
    bool fmudaNota(const QString &rid, const QString &rnota);
    bool fmudaProgresso(const QString &rid, const QString &rprogresso);
    bool fdeletedaLista(const QString &rid);
    bool finsereNomeAlternativo(const QString &rid, const QStringList &rnome);

    //TODO - Funções de get da midia - Ex: Get Name
};
#endif // IMEDIAMANAGER_H
