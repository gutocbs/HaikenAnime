#include "SqliteMediaMapper.h"

#include <QJsonArray>
#include <QJsonDocument>

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
    media.CoverUrl = query.value(10).toString();
    media.CoverMediumUrl = query.value(11).toString();
    media.CoverLargeUrl = query.value(12).toString();
    media.CoverExtraLargeUrl = query.value(13).toString();
    media.Synopsis = query.value(14).toString();
    media.Type = static_cast<MediaType>(query.value(15).toInt());
    media.Status = static_cast<MediaStatus>(query.value(16).toInt());
    media.ListStatus = static_cast<UserListStatus>(query.value(17).toInt());
    return media;
}
