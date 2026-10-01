#include "AniListMediaMapper.h"

#include <QJsonArray>
#include <QRegularExpression>
#include <QSet>
#include <QTextDocumentFragment>
#include <QUrl>
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

std::optional<qint64> OptionalInteger64(const QJsonValue &value) {
    if (!value.isDouble()) {
        return std::nullopt;
    }
    const double number = value.toDouble();
    const auto integer = static_cast<qint64>(number);
    if (number != static_cast<double>(integer)) {
        return std::nullopt;
    }
    return integer;
}

void AppendLinks(const QJsonValue &value, QList<AniListMediaLinkDto> &links) {
    for (const auto &entry : value.toArray()) {
        if (!entry.isObject()) {
            continue;
        }
        const auto object = entry.toObject();
        links.append({object.value(QStringLiteral("site")).toString(),
                      object.value(QStringLiteral("url")).toString()});
    }
}

QString NormalizeSynopsis(QString html) {
    if (html.trimmed().isEmpty()) {
        return {};
    }
    html.replace(QRegularExpression(QStringLiteral("<\\s*/\\s*(p|div)\\s*>"),
                                    QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral("<br><br>"));
    html.remove(QRegularExpression(QStringLiteral("<\\s*(p|div)(?:\\s+[^>]*)?>"),
                                   QRegularExpression::CaseInsensitiveOption));
    html.replace(QRegularExpression(QStringLiteral("<\\s*br\\s*/?\\s*>"),
                                    QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral("<br>"));
    const auto text = QTextDocumentFragment::fromHtml(html).toPlainText();
    QString normalized = text;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(u'\r', u'\n');
    normalized.replace(QRegularExpression(QStringLiteral("[ \\t]+\\n")), QStringLiteral("\n"));
    normalized.replace(QRegularExpression(QStringLiteral("\\n[ \\t]+")), QStringLiteral("\n"));
    normalized.replace(QRegularExpression(QStringLiteral("\\n{3,}")), QStringLiteral("\n\n"));
    return normalized.trimmed();
}

QString ValidHttpUrl(const QString &candidate) {
    QUrl url(candidate.trimmed(), QUrl::StrictMode);
    const auto scheme = url.scheme().toLower();
    if (!url.isValid() || url.isRelative() || url.host().isEmpty()
        || (scheme != QStringLiteral("http") && scheme != QStringLiteral("https"))) {
        return {};
    }
    url.setScheme(scheme);
    url.setHost(url.host().toLower());
    if ((scheme == QStringLiteral("http") && url.port() == 80)
        || (scheme == QStringLiteral("https") && url.port() == 443)) {
        url.setPort(-1);
    }
    url = url.adjusted(QUrl::NormalizePathSegments | QUrl::RemoveFragment);
    return url.toString(QUrl::FullyEncoded);
}

QList<MediaLink> NormalizeLinks(const QList<AniListMediaLinkDto> &externalLinks) {
    QList<MediaLink> links;
    QSet<QString> seen;
    for (const auto &candidate : externalLinks) {
        const auto site = candidate.site.simplified();
        const auto url = ValidHttpUrl(candidate.url);
        if (site.isEmpty() || url.isEmpty()) {
            continue;
        }
        const auto key = site.toCaseFolded() + u'\n' + url;
        if (seen.contains(key)) {
            continue;
        }
        seen.insert(key);
        links.append({site, url});
    }
    return links;
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
    media.listStatus = object.value(QStringLiteral("list")).toString();
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
    media.listStatus = object.value(QStringLiteral("userListStatus")).toString();
    const auto title = object.value(QStringLiteral("title")).toObject();
    media.titleRomaji = title.value(QStringLiteral("romaji")).toString();
    media.titleEnglish = title.value(QStringLiteral("english")).toString();
    media.titleNative = title.value(QStringLiteral("native")).toString();
    for (const auto &synonym : object.value(QStringLiteral("synonyms")).toArray()) {
        media.titleSynonyms.append(synonym.toString());
    }
    media.episodes = OptionalInteger(object.value(QStringLiteral("episodes")));
    media.chapters = OptionalInteger(object.value(QStringLiteral("chapters")));
    media.progress = OptionalInteger(object.value(QStringLiteral("progress")));
    media.personalScore = OptionalInteger(object.value(QStringLiteral("score")));
    media.averageScore = OptionalInteger(object.value(QStringLiteral("averageScore")));
    const auto coverImage = object.value(QStringLiteral("coverImage")).toObject();
    media.coverImages.medium = coverImage.value(QStringLiteral("medium")).toString();
    media.coverImages.large = coverImage.value(QStringLiteral("large")).toString();
    media.coverImages.extraLarge = coverImage.value(QStringLiteral("extraLarge")).toString();
    media.coverImageUrl = SelectCoverUrl(media.coverImages, CoverQuality::Medium);
    media.description = object.value(QStringLiteral("description")).toString();
    media.season = object.value(QStringLiteral("season")).toString();
    media.seasonYear = OptionalInteger(object.value(QStringLiteral("seasonYear")));
    const auto nextAiringEpisode = object.value(QStringLiteral("nextAiringEpisode")).toObject();
    media.nextAiringEpisode = OptionalInteger(nextAiringEpisode.value(QStringLiteral("episode")));
    media.nextAiringAt = OptionalInteger64(nextAiringEpisode.value(QStringLiteral("airingAt")));
    media.siteUrl = object.value(QStringLiteral("siteUrl")).toString();
    AppendLinks(object.value(QStringLiteral("externalLinks")), media.externalLinks);
    AppendLinks(object.value(QStringLiteral("streamingEpisodes")), media.externalLinks);
    return media;
}

QString AniListMediaMapper::SelectCoverUrl(const AniListCoverImagesDto &images,
                                           const CoverQuality quality) {
    switch (quality) {
    case CoverQuality::Medium:
        if (!images.medium.isEmpty()) {
            return images.medium;
        }
        return images.large.isEmpty() ? images.extraLarge : images.large;
    case CoverQuality::Large:
        if (!images.large.isEmpty()) {
            return images.large;
        }
        return images.medium.isEmpty() ? images.extraLarge : images.medium;
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
    media.ConsumedChapters = externalMedia.progress.value_or(0);
    media.PersonalScore = externalMedia.personalScore.value_or(0);
    media.AverageScore = externalMedia.averageScore.value_or(0);
    media.CoverUrl = externalMedia.coverImageUrl;
    media.CoverMediumUrl = externalMedia.coverImages.medium;
    media.CoverLargeUrl = externalMedia.coverImages.large;
    media.CoverExtraLargeUrl = externalMedia.coverImages.extraLarge;
    media.Synopsis = NormalizeSynopsis(externalMedia.description);
    media.Season = externalMedia.season;
    media.SeasonYear = externalMedia.seasonYear;
    media.NextAiringEpisode = externalMedia.nextAiringEpisode;
    media.NextAiringAt = externalMedia.nextAiringAt;
    media.AniListUrl = ValidHttpUrl(externalMedia.siteUrl);
    media.ExternalLinks = NormalizeLinks(externalMedia.externalLinks);

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

    const auto listStatus = externalMedia.listStatus.trimmed().toUpper();
    if (listStatus == QStringLiteral("CURRENT")) {
        media.ListStatus = UserListStatus::Current;
    } else if (listStatus == QStringLiteral("PLANNING")
               || listStatus == QStringLiteral("PLAN TO WATCH")
               || listStatus == QStringLiteral("PLAN TO READ")) {
        media.ListStatus = UserListStatus::Planning;
    } else if (listStatus == QStringLiteral("PAUSED") || listStatus == QStringLiteral("ON_HOLD")) {
        media.ListStatus = UserListStatus::OnHold;
    } else if (listStatus == QStringLiteral("DROPPED")) {
        media.ListStatus = UserListStatus::Dropped;
    } else if (listStatus == QStringLiteral("COMPLETED")) {
        media.ListStatus = UserListStatus::Completed;
    } else {
        media.ListStatus = UserListStatus::Unknown;
    }
    return media;
}
