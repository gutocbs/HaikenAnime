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

AniListMediaDto AniListMediaMapper::fromGraphQlJson(const QJsonObject &object) {
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
