#include "AniListMediaMapper.h"

#include <QJsonArray>
#include <QtMath>

namespace {

std::optional<int> OptionalInteger(const QJsonValue &value) {
    if (value.isNull() || value.isUndefined()) {
        return std::nullopt;
    }
    return value.toInt();
}

std::optional<int> OptionalInteger(const QString &value) {
    if (value.isEmpty()) {
        return std::nullopt;
    }
    return value.toInt();
}

}

AniListMediaDto AniListMediaMapper::fromFixtureJson(const QJsonObject &object) {
    AniListMediaDto media;
    media.id = object.value(QStringLiteral("id")).toString().toInt();
    media.mediaFormat = object.value(QStringLiteral("format")).toString();
    media.mediaType = media.mediaFormat.compare(QStringLiteral("TV"), Qt::CaseInsensitive) == 0
        ? QStringLiteral("ANIME")
        : QStringLiteral("MANGA");
    media.status = object.value(QStringLiteral("status")).toString();
    media.titleRomaji = object.value(QStringLiteral("title")).toString();
    media.titleEnglish = object.value(QStringLiteral("englishTitle")).toString();
    media.titleNative = object.value(QStringLiteral("nativeTitle")).toString();
    media.titleSynonyms = object.value(QStringLiteral("alternativeTitles")).toVariant().toStringList();
    media.episodes = OptionalInteger(object.value(QStringLiteral("totalEpisodes")).toString());
    const auto siteScore = object.value(QStringLiteral("siteScore")).toString();
    if (!siteScore.isEmpty()) {
        media.averageScore = qRound(siteScore.toDouble());
    }
    media.coverImageUrl = object.value(QStringLiteral("coverImageUrl")).toString();
    media.description = object.value(QStringLiteral("synopsis")).toString();
    return media;
}

AniListMediaDto AniListMediaMapper::FromGraphQlJson(const QJsonObject &object) {
    AniListMediaDto media;
    media.id = object.value(QStringLiteral("id")).toInt();
    media.mediaType = object.value(QStringLiteral("type")).toString();
    media.mediaFormat = object.value(QStringLiteral("format")).toString();
    media.status = object.value(QStringLiteral("status")).toString();
    const auto title = object.value(QStringLiteral("title")).toObject();
    media.titleRomaji = title.value(QStringLiteral("romaji")).toString();
    media.titleEnglish = title.value(QStringLiteral("english")).toString();
    media.titleNative = title.value(QStringLiteral("native")).toString();
    for (const auto &synonym : object.value(QStringLiteral("synonyms")).toArray()) {
        media.titleSynonyms.append(synonym.toString());
    }
    media.episodes = OptionalInteger(object.value(QStringLiteral("episodes")));
    media.chapters = OptionalInteger(object.value(QStringLiteral("chapters")));
    media.averageScore = OptionalInteger(object.value(QStringLiteral("averageScore")));
    const auto coverImage = object.value(QStringLiteral("coverImage")).toObject();
    media.coverImages.medium = coverImage.value(QStringLiteral("medium")).toString();
    media.coverImages.large = coverImage.value(QStringLiteral("large")).toString();
    media.coverImages.extraLarge = coverImage.value(QStringLiteral("extraLarge")).toString();
    media.coverImageUrl = SelectCoverUrl(media.coverImages, CoverQuality::Medium);
    media.description = object.value(QStringLiteral("description")).toString();
    return media;
}

QString AniListMediaMapper::SelectCoverUrl(const AniListCoverImagesDto &images,
                                           const CoverQuality quality) {
    switch (quality) {
    case CoverQuality::Medium:
        return images.medium;
    case CoverQuality::Large:
        return images.large.isEmpty() ? images.medium : images.large;
    case CoverQuality::ExtraLarge:
        if (!images.extraLarge.isEmpty()) {
            return images.extraLarge;
        }
        return images.large.isEmpty() ? images.medium : images.large;
    }
    return {};
}

Media AniListMediaMapper::ToDomainMedia(const AniListMediaDto &externalMedia) {
    Media media;
    media.Id = externalMedia.id;
    media.Name = externalMedia.titleRomaji;
    media.EnglishName = externalMedia.titleEnglish;
    media.OriginalName = externalMedia.titleNative;
    media.AlternativeNames = externalMedia.titleSynonyms;
    const int episodes = externalMedia.episodes.value_or(0);
    media.TotalChapters = episodes > 0 ? episodes : externalMedia.chapters.value_or(0);
    media.AverageScore = externalMedia.averageScore.value_or(0);
    media.CoverUrl = externalMedia.coverImageUrl;
    media.Synopsis = externalMedia.description;

    if (externalMedia.mediaFormat.compare(QStringLiteral("NOVEL"), Qt::CaseInsensitive) == 0) {
        media.Type = MediaType::Novel;
    } else if (externalMedia.mediaType.compare(QStringLiteral("MANGA"), Qt::CaseInsensitive) == 0) {
        media.Type = MediaType::Manga;
    } else if (externalMedia.mediaType.compare(QStringLiteral("ANIME"), Qt::CaseInsensitive) == 0) {
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
