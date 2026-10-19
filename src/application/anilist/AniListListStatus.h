#ifndef HAIKENANIME_ANILISTLISTSTATUS_H
#define HAIKENANIME_ANILISTLISTSTATUS_H

#include <QString>
#include <QStringList>

#include <optional>

#include "../../domain/media/UserListStatus.h"

/** AniList MediaListStatus name ("CURRENT", "PAUSED", …); empty for Unknown. */
inline QString AniListListStatusName(const UserListStatus status) {
    switch (status) {
    case UserListStatus::Current: return QStringLiteral("CURRENT");
    case UserListStatus::Planning: return QStringLiteral("PLANNING");
    case UserListStatus::OnHold: return QStringLiteral("PAUSED");
    case UserListStatus::Dropped: return QStringLiteral("DROPPED");
    case UserListStatus::Completed: return QStringLiteral("COMPLETED");
    case UserListStatus::Unknown: break;
    }
    return {};
}

/** Inverse of AniListListStatusName, ignoring case and surrounding spaces; nullopt for any other value. */
inline std::optional<UserListStatus> ParseAniListListStatus(const QString &value) {
    const auto normalized = value.trimmed().toUpper();
    for (const auto status : {UserListStatus::Current, UserListStatus::Planning, UserListStatus::OnHold,
                              UserListStatus::Dropped, UserListStatus::Completed}) {
        if (normalized == AniListListStatusName(status)) return status;
    }
    return std::nullopt;
}

/** AniList statuses of the enabled personal lists (UserListKeys), in order; unknown keys are skipped. */
inline QStringList AniListStatusesForEnabledUserLists(const QStringList &keys) {
    QStringList statuses;
    for (const auto &key : keys) {
        const auto status = UserListStatusFromKey(key);
        if (status != UserListStatus::Unknown) statuses.append(AniListListStatusName(status));
    }
    return statuses;
}

#endif // HAIKENANIME_ANILISTLISTSTATUS_H
