#ifndef ENUMS_H
#define ENUMS_H

#include <QObject>

class Enums : public QObject{
public:
    enum hashList{LISTA, POSICAO, NOME, NOMEALTERNATIVO};
    Q_ENUM(hashList)
    enum mediaType{ANIME, MANGA, NOVEL, SEASON};
    Q_ENUM(mediaType)
    enum mediaList{CURRENT, COMPLETED, PAUSED, DROPPED, PLANNING, SEARCH, WINTER, SUMMER, FALL, SPRING};
    Q_ENUM(mediaList)
    virtual ~Enums() = 0;
};


#endif // ENUMS_H
