//
// Created by gutocbs on 22/09/2026.
//

#ifndef HAIKENANIME_MEDIA_H
#define HAIKENANIME_MEDIA_H
#include <QString>
#include <QStringList>
#include <optional>

#include "MediaLink.h"
#include "MediaStatus.h"
#include "MediaType.h"
#include "UserListStatus.h"


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
    QString CoverMediumUrl;
    QString CoverLargeUrl;
    QString CoverExtraLargeUrl;
    QString Synopsis;
    QString Season;
    std::optional<int> SeasonYear;
    std::optional<int> NextAiringEpisode;
    std::optional<qint64> NextAiringAt;
    QString AniListUrl;
    QList<MediaLink> ExternalLinks;
    MediaStatus Status = MediaStatus::Unknown;
    UserListStatus ListStatus = UserListStatus::Unknown;
    MediaType Type = MediaType::Unknown;
};


#endif //HAIKENANIME_MEDIA_H
