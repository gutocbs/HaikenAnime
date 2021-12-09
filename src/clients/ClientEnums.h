#ifndef CLIENTENUMS_H
#define CLIENTENUMS_H

#include <QObject>

class ClientEnums : public QObject
{
    Q_OBJECT
public:
    enum clients{ANILIST};
    Q_ENUM(clients)
    enum updateType{SCORE, PROGRESS, LIST, DELETE};
    Q_ENUM(updateType)
};

#endif // CLIENTENUMS_H
