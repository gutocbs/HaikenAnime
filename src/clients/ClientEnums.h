#ifndef CLIENTENUMS_H
#define CLIENTENUMS_H

#include <QObject>
#include <QMetaEnum>
#include <QVariant>

class ClientEnums : public QObject
{
    Q_OBJECT
public:
    enum clients{ANILIST};
    Q_ENUM(clients)
    enum updateType{SCORE, PROGRESS, LIST, DELETE};
    Q_ENUM(updateType)
    static clients QStringToClient(QString enumName){
        if(enumName.compare(enumClientToQString(clients::ANILIST), Qt::CaseInsensitive) == 0) return clients::ANILIST;
        return clients::ANILIST;
    }

    static QString enumClientToQString(clients enumName){
        return QVariant::fromValue(enumName).toString();
    }
};

#endif // CLIENTENUMS_H
