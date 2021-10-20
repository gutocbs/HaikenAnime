#ifndef CLIENTENUMS_H
#define CLIENTENUMS_H

#include <QObject>

class ClientEnums : public QObject
{
    Q_OBJECT
public:
    enum updateType{SCORE, PROGRESS, LIST};
    Q_ENUM(updateType)
};

#endif // CLIENTENUMS_H
