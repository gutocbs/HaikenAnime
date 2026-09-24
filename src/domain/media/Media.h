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
    int Id = 0;
    QString Name;
    QString EnglishName;
    QString OriginalName;
    QStringList AlternativeNames;
    int TotalChapters = 0;
    int ConsumedChapters = 0;
    int NextChapter = 0;
    int AverageScore = 0;
    int PersonalScore = 0;
    QString CoverUrl;
    QString Synopsis;
    MediaStatus Status = MediaStatus::Unknown;
    MediaType Type = MediaType::Unknown;
};


#endif //HAIKENANIME_MEDIA_H
