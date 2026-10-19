#include "SqliteMediaMapper.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>

namespace {
QVariant OptionalValue(const std::optional<int> &value) {
    return value.has_value() ? QVariant(value.value()) : QVariant();
}

QVariant OptionalValue(const std::optional<qint64> &value) {
    return value.has_value() ? QVariant::fromValue(value.value()) : QVariant();
}

QString SerializeLinks(const QList<MediaLink> &links) {
    QJsonArray array;
    for (const auto &link : links) {
        array.append(QJsonObject{{QStringLiteral("site"), link.Site},
                                 {QStringLiteral("url"), link.Url}});
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QList<MediaLink> ParseLinks(const QByteArray &json) {
    QList<MediaLink> links;
    for (const auto &value : QJsonDocument::fromJson(json).array()) {
        const auto link = value.toObject();
        const auto site = link.value(QStringLiteral("site"));
        const auto url = link.value(QStringLiteral("url"));
        if (site.isString() && url.isString()) links.append(MediaLink{site.toString(), url.toString()});
    }
    return links;
}

QString NonNullLocalPath(const Media &media) {
    return media.LocalPath.isNull() ? QStringLiteral("") : media.LocalPath;
}
}

Media SqliteMediaMapper::Map(const QSqlQuery &query) {
    Media media;
    media.Id = query.value(0).toInt();
    media.Name = query.value(1).toString();
    media.EnglishName = query.value(2).toString();
    media.OriginalName = query.value(3).toString();
    media.AlternativeNames = ParseNames(query.value(4).toString());
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
    media.ExternalLinks = ParseLinks(query.value(24).toByteArray());
    return media;
}

void SqliteMediaMapper::BindUpsert(QSqlQuery &query, const Media &media, const QStringList &alternativeNames) {
    query.bindValue(QStringLiteral(":id"), media.Id);
    query.bindValue(QStringLiteral(":name"), media.Name);
    query.bindValue(QStringLiteral(":english_name"), media.EnglishName);
    query.bindValue(QStringLiteral(":original_name"), media.OriginalName);
    query.bindValue(QStringLiteral(":alternative_names"), SerializeNames(alternativeNames));
    query.bindValue(QStringLiteral(":total_chapters"), media.TotalChapters);
    query.bindValue(QStringLiteral(":consumed_chapters"), media.ConsumedChapters);
    query.bindValue(QStringLiteral(":average_score"), media.AverageScore);
    query.bindValue(QStringLiteral(":personal_score"), media.PersonalScore);
    query.bindValue(QStringLiteral(":cover_url"), media.CoverUrl);
    query.bindValue(QStringLiteral(":cover_medium_url"), media.CoverMediumUrl);
    query.bindValue(QStringLiteral(":cover_large_url"), media.CoverLargeUrl);
    query.bindValue(QStringLiteral(":cover_extra_large_url"), media.CoverExtraLargeUrl);
    query.bindValue(QStringLiteral(":synopsis"), media.Synopsis);
    query.bindValue(QStringLiteral(":type"), static_cast<int>(media.Type));
    query.bindValue(QStringLiteral(":status"), static_cast<int>(media.Status));
    query.bindValue(QStringLiteral(":user_list_status"), static_cast<int>(media.ListStatus));
    query.bindValue(QStringLiteral(":local_path"), NonNullLocalPath(media));
    query.bindValue(QStringLiteral(":season"), media.Season);
    query.bindValue(QStringLiteral(":season_year"), OptionalValue(media.SeasonYear));
    query.bindValue(QStringLiteral(":next_airing_episode"), OptionalValue(media.NextAiringEpisode));
    query.bindValue(QStringLiteral(":next_airing_at"), OptionalValue(media.NextAiringAt));
    query.bindValue(QStringLiteral(":anilist_url"), media.AniListUrl);
    query.bindValue(QStringLiteral(":external_links"), SerializeLinks(media.ExternalLinks));
}

void SqliteMediaMapper::BindPersonalListUpdate(QSqlQuery &query, const Media &media) {
    query.bindValue(QStringLiteral(":id"), media.Id);
    query.bindValue(QStringLiteral(":consumed_chapters"), media.ConsumedChapters);
    query.bindValue(QStringLiteral(":personal_score"), media.PersonalScore);
    query.bindValue(QStringLiteral(":user_list_status"), static_cast<int>(media.ListStatus));
    query.bindValue(QStringLiteral(":local_path"), NonNullLocalPath(media));
    query.bindValue(QStringLiteral(":alternative_names"), SerializeNames(media.AlternativeNames));
}

QString SqliteMediaMapper::SerializeNames(const QStringList &names) {
    return QString::fromUtf8(QJsonDocument::fromVariant(names).toJson());
}

QStringList SqliteMediaMapper::ParseNames(const QString &json) {
    QStringList names;
    for (const auto &value : QJsonDocument::fromJson(json.toUtf8()).array()) {
        if (value.isString()) names.append(value.toString());
    }
    return names;
}
