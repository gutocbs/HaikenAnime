#ifndef HAIKENANIME_ANILISTPENDINGCHANGE_H
#define HAIKENANIME_ANILISTPENDINGCHANGE_H

#include <QDateTime>
#include <QString>

#include <variant>

#include "AniListField.h"
#include "AniListPendingChangeStatus.h"

using AniListFieldValue = std::variant<QString, int>;

/** Represents one local field change that may need to be sent to AniList. */
struct AniListPendingChange {
    int mediaId = 0;
    AniListField field = AniListField::ExternalId;
    AniListFieldValue previousValue = QString();
    AniListFieldValue newValue = QString();
    QDateTime createdAt;
    QDateTime localUpdatedAt;
    QDateTime remoteObservedAt;
    QString remoteVersion;
    int attempts = 0;
    AniListPendingChangeStatus status = AniListPendingChangeStatus::Pending;
    QString lastError;
};

#endif // HAIKENANIME_ANILISTPENDINGCHANGE_H
