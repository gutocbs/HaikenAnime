#include "AniListMediaMapper.h"

#include <QJsonArray>
#include <QtMath>

AniListMediaDto AniListMediaMapper::fromFixtureJson(const QJsonObject &object) {
    AniListMediaDto media;
    media.id = object.value(QStringLiteral("id")).toString().toInt();
    media.type = object.value(QStringLiteral("format")).toString();
    media.status = object.value(QStringLiteral("status")).toString();
    media.titleRomaji = object.value(QStringLiteral("title")).toString();
    media.titleEnglish = object.value(QStringLiteral("englishTitle")).toString();
    media.titleNative = object.value(QStringLiteral("nativeTitle")).toString();
    media.titleSynonyms = object.value(QStringLiteral("alternativeTitles")).toVariant().toStringList();
    media.episodes = object.value(QStringLiteral("totalEpisodes")).toString().toInt();
    media.averageScore = qRound(object.value(QStringLiteral("siteScore")).toString().toDouble());
    media.coverImageUrl = object.value(QStringLiteral("coverImageUrl")).toString();
    media.description = object.value(QStringLiteral("synopsis")).toString();
    return media;
}

AniListMediaDto AniListMediaMapper::FromGraphQlJson(const QJsonObject &object) {
    AniListMediaDto media;
    media.id = object.value(QStringLiteral("id")).toInt();
    media.type = object.value(QStringLiteral("type")).toString();
    media.status = object.value(QStringLiteral("status")).toString();
    const auto title = object.value(QStringLiteral("title")).toObject();
    media.titleRomaji = title.value(QStringLiteral("romaji")).toString();
    media.titleEnglish = title.value(QStringLiteral("english")).toString();
    media.titleNative = title.value(QStringLiteral("native")).toString();
    for (const auto &synonym : title.value(QStringLiteral("synonyms")).toArray()) {
        media.titleSynonyms.append(synonym.toString());
    }
    media.episodes = object.value(QStringLiteral("episodes")).toInt();
    media.chapters = object.value(QStringLiteral("chapters")).toInt();
    media.averageScore = object.value(QStringLiteral("averageScore")).toInt();
    media.coverImageUrl = object.value(QStringLiteral("coverImage")).toObject()
                              .value(QStringLiteral("large")).toString();
    media.description = object.value(QStringLiteral("description")).toString();
    return media;
}

Media AniListMediaMapper::ToDomainMedia(const AniListMediaDto &externalMedia) {
    Media media;
    media.Id = externalMedia.id;
    media.Name = externalMedia.titleRomaji;
    media.EnglishName = externalMedia.titleEnglish;
    media.OriginalName = externalMedia.titleNative;
    media.AlternativeNames = externalMedia.titleSynonyms;
    media.TotalChapters = externalMedia.episodes > 0 ? externalMedia.episodes : externalMedia.chapters;
    media.AverageScore = externalMedia.averageScore;
    media.CoverUrl = externalMedia.coverImageUrl;
    media.Synopsis = externalMedia.description;

    if (externalMedia.type.compare(QStringLiteral("MANGA"), Qt::CaseInsensitive) == 0) {
        media.Type = MediaType::Manga;
    } else if (externalMedia.type.compare(QStringLiteral("NOVEL"), Qt::CaseInsensitive) == 0) {
        media.Type = MediaType::Novel;
    } else if (externalMedia.type.compare(QStringLiteral("ANIME"), Qt::CaseInsensitive) == 0
               || externalMedia.type.compare(QStringLiteral("TV"), Qt::CaseInsensitive) == 0) {
        media.Type = MediaType::Anime;
    } else {
        media.Type = MediaType::Unknown;
    }

    if (externalMedia.status.compare(QStringLiteral("RELEASING"), Qt::CaseInsensitive) == 0
        || externalMedia.status.compare(QStringLiteral("Releasing"), Qt::CaseInsensitive) == 0) {
        media.Status = MediaStatus::Releasing;
    } else if (externalMedia.status.compare(QStringLiteral("FINISHED"), Qt::CaseInsensitive) == 0
               || externalMedia.status.compare(QStringLiteral("Finished Airing"), Qt::CaseInsensitive) == 0
               || externalMedia.status.compare(QStringLiteral("RELEASED"), Qt::CaseInsensitive) == 0) {
        media.Status = MediaStatus::Released;
    } else if (externalMedia.status.compare(QStringLiteral("NOT_YET_RELEASED"), Qt::CaseInsensitive) == 0) {
        media.Status = MediaStatus::NotReleased;
    } else {
        media.Status = MediaStatus::Unknown;
    }
    return media;
}
