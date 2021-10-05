#ifndef MEDIA_H
#define MEDIA_H

#include <QObject>
#include <QStringList>
#include <QDate> //Pega data de estreia e season
#include "src/utilities/Enums.h"

class Media : public QObject
{
    Q_OBJECT
public:
    explicit Media(QObject *parent = nullptr){
        Q_UNUSED(parent)
        complete = false;
        seasonNumber = 0;
    }
    int id;
    QString originalName;
    QString englishName;
    QString japaneseName;
    QStringList customNames;
    int totalEpisodes;
    int progress;
    int nextNewEpisode;
    QString nextAiringEpisodeDate;//MON 23 13:00
    QString meanScore;
    QString coverURL;
    QString personalScore;
    QString status;
    QString synopsis;
    QString yearSeason;
    QString cleanSeason;
    Enums::mediaList mediaList; //Esse atributo será usado para a função de busca e para função de mudar a lista do anime
    Enums::mediaType format; //Usado pra organizar a lista
    QString siteURL;
    QDate startDate;
    bool complete;
    int seasonNumber;
    int mediaListIndex;
    QString mediaPath;
};

#endif // MEDIA_H
