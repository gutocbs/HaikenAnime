#include "SqliteMediaMapper.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

Media SqliteMediaMapper::Map(const QSqlQuery &query) {
    Media media;
    media.Id = query.value(0).toInt();
    media.Name = query.value(1).toString();
    media.EnglishName = query.value(2).toString();
    media.OriginalName = query.value(3).toString();
    const auto alternativeNames = QJsonDocument::fromJson(query.value(4).toByteArray());
    for (const auto &name : alternativeNames.array()) {
        if (name.isString()) {
            media.AlternativeNames.append(name.toString());
        }
    }
    media.TotalChapters = query.value(5).toInt();
    media.ConsumedChapters = query.value(6).toInt();
    media.NextChapter = query.value(7).toInt();
    media.AverageScore = query.value(8).toInt();
    media.PersonalScore = query.value(9).toInt();
    media.LocalPath = query.value(10).toString();
    media.CoverUrl = query.value(11).toString();
    media.CoverMediumUrl = query.value(12).toString();
    media.CoverLargeUrl = query.value(13).toString();
    media.CoverExtraLargeUrl = query.value(14).toString();
    media.Synopsis = query.value(15).toString();
    media.Type = static_cast<MediaType>(query.value(16).toInt());
    media.Status = static_cast<MediaStatus>(query.value(17).toInt());
    media.ListStatus = static_cast<UserListStatus>(query.value(18).toInt());
    media.Season = query.value(19).toString();
    if (!query.value(20).isNull()) media.SeasonYear = query.value(20).toInt();
    if (!query.value(21).isNull()) media.NextAiringEpisode = query.value(21).toInt();
    if (!query.value(22).isNull()) media.NextAiringAt = query.value(22).toLongLong();
    media.AniListUrl = query.value(23).toString();
    const auto externalLinks = QJsonDocument::fromJson(query.value(24).toByteArray());
    for (const auto &value : externalLinks.array()) {
        const auto link = value.toObject();
        const auto site = link.value(QStringLiteral("site"));
        const auto url = link.value(QStringLiteral("url"));
        if (site.isString() && url.isString()) {
            media.ExternalLinks.append(MediaLink{site.toString(), url.toString()});
        }
    }
    return media;
}
