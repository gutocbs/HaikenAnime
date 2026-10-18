#include "MediaPresentation.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QSet>
#include <QTimeZone>

#include <algorithm>

QString FormatProgress(const int consumed, const int total) {
    const auto totalLabel = total > 0 ? QString::number(total) : QStringLiteral("?");
    return QStringLiteral("%1/%2").arg(std::max(consumed, 0)).arg(totalLabel);
}

QString MediaStatusLabel(const MediaStatus status) {
    switch (status) {
    case MediaStatus::NotReleased: return QCoreApplication::translate("HomeMediaModel", "Ainda não lançado");
    case MediaStatus::Releasing: return QCoreApplication::translate("HomeMediaModel", "Em lançamento");
    case MediaStatus::Released: return QCoreApplication::translate("HomeMediaModel", "Concluído");
    case MediaStatus::Unknown:
    default: return QCoreApplication::translate("HomeMediaModel", "Desconhecido");
    }
}

QString MediaTypeLabel(const MediaType type) {
    switch (type) {
    case MediaType::Anime: return QStringLiteral("Anime");
    case MediaType::Manga: return QStringLiteral("Manga");
    case MediaType::Novel: return QStringLiteral("Novel");
    case MediaType::Unknown:
    default: return QCoreApplication::translate("HomeMediaModel", "Desconhecido");
    }
}

QString PresentMediaSeason(const Media &media) {
    QString season;
    if (media.Season.compare(QStringLiteral("WINTER"), Qt::CaseInsensitive) == 0) {
        season = QCoreApplication::translate("HomeScreenController", "Inverno");
    } else if (media.Season.compare(QStringLiteral("SPRING"), Qt::CaseInsensitive) == 0) {
        season = QCoreApplication::translate("HomeScreenController", "Primavera");
    } else if (media.Season.compare(QStringLiteral("SUMMER"), Qt::CaseInsensitive) == 0) {
        season = QCoreApplication::translate("HomeScreenController", "Verão");
    } else if (media.Season.compare(QStringLiteral("FALL"), Qt::CaseInsensitive) == 0) {
        season = QCoreApplication::translate("HomeScreenController", "Outono");
    } else {
        season = media.Season.simplified();
    }
    if (season.isEmpty()) return media.SeasonYear ? QString::number(*media.SeasonYear) : QString();
    return media.SeasonYear
        ? QCoreApplication::translate("HomeScreenController", "%1 de %2").arg(season).arg(*media.SeasonYear)
        : season;
}

QString PresentMediaNextAiring(const Media &media) {
    if (!media.NextAiringEpisode) return {};
    const QString episode = QCoreApplication::translate("HomeScreenController", "Episódio %1")
                                .arg(*media.NextAiringEpisode);
    if (!media.NextAiringAt) return episode;
    const auto airingAt = QDateTime::fromSecsSinceEpoch(*media.NextAiringAt, QTimeZone::utc())
                              .toString(QStringLiteral("dd/MM/yyyy HH:mm 'UTC'"));
    return QStringLiteral("%1 · %2").arg(episode, airingAt);
}

QVariantList PresentMediaLinks(const Media &media) {
    QVariantList links;
    QSet<QString> seen;
    const auto appendLink = [&links, &seen](const QString &site, const QString &url) {
        const QString key = site.toCaseFolded();
        if (site.isEmpty() || url.isEmpty() || seen.contains(key)) return;
        seen.insert(key);
        links.append(QVariantMap{{QStringLiteral("site"), site}, {QStringLiteral("url"), url}});
    };
    appendLink(QStringLiteral("AniList"), media.AniListUrl);
    for (const auto &link : media.ExternalLinks) appendLink(link.Site, link.Url);
    return links;
}

QVariantMap PresentMediaDetails(const Media &media, const QString &title, const QString &coverSource) {
    return {{QStringLiteral("selectedTitle"), title},
            {QStringLiteral("selectedSynopsis"), media.Synopsis},
            {QStringLiteral("selectedTypeLabel"), MediaTypeLabel(media.Type)},
            {QStringLiteral("selectedStatusLabel"), MediaStatusLabel(media.Status)},
            {QStringLiteral("selectedProgress"), FormatProgress(media.ConsumedChapters, media.TotalChapters)},
            {QStringLiteral("selectedScore"),
             media.PersonalScore > 0 ? QString::number(media.PersonalScore) : QStringLiteral("—")},
            {QStringLiteral("selectedAverageScore"),
             media.AverageScore > 0 ? QString::number(media.AverageScore) : QStringLiteral("—")},
            {QStringLiteral("selectedSeasonLabel"), PresentMediaSeason(media)},
            {QStringLiteral("selectedNextAiringLabel"), PresentMediaNextAiring(media)},
            {QStringLiteral("selectedMediaLinks"), PresentMediaLinks(media)},
            {QStringLiteral("selectedAlternativeNames"), media.AlternativeNames},
            {QStringLiteral("selectedCoverSource"), coverSource},
            {QStringLiteral("selectedMediaInPersonalList"), media.ListStatus != UserListStatus::Unknown}};
}
