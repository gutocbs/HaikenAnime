#ifndef HAIKENANIME_CARDSTATUSPRESENTATION_H
#define HAIKENANIME_CARDSTATUSPRESENTATION_H

#include <QString>

#include <optional>

enum class CardStatusPresentation { PersonalListStatus, MediaReleaseStatus };

inline QString CardStatusPresentationKey(const CardStatusPresentation presentation) {
    switch (presentation) {
    case CardStatusPresentation::PersonalListStatus:
        return QStringLiteral("personal-list-status");
    case CardStatusPresentation::MediaReleaseStatus:
        return QStringLiteral("media-release-status");
    }
    return {};
}

inline std::optional<CardStatusPresentation> ParseCardStatusPresentation(const QString &key) {
    if (key == QStringLiteral("personal-list-status"))
        return CardStatusPresentation::PersonalListStatus;
    if (key == QStringLiteral("media-release-status"))
        return CardStatusPresentation::MediaReleaseStatus;
    return std::nullopt;
}

#endif // HAIKENANIME_CARDSTATUSPRESENTATION_H
