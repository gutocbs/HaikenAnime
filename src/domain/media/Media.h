//
// Created by gutocbs on 22/09/2026.
//

#ifndef HAIKENANIME_MEDIA_H
#define HAIKENANIME_MEDIA_H
#include <QString>
#include <QStringList>

#include "MediaStatus.h"
#include "MediaType.h"


class Media {
public:
    int Id;
    QString Name;
    QString EnglishName;
    QString OriginalName;
    QStringList AlternativeNames;
    int TotalChapters;
    int ConsumedChapters;
    int NextChapter;
    int AverageScore;
    int PersonalScore;
    QString CoverUrl;
    QString Synopsis;
    MediaStatus Status;
    MediaType Type;
};


#endif //HAIKENANIME_MEDIA_H
