#include "AniListPendingChangeFactory.h"

bool AniListPendingChangeFactory::Create(const int mediaId, const AniListField field,
                                         const AniListFieldValue &previousValue,
                                         const AniListFieldValue &newValue,
                                         AniListPendingChange &change, QString &error) {
    if (mediaId <= 0) {
        error = QStringLiteral("A pending AniList change requires a positive media identifier.");
        return false;
    }

    change = {};
    change.mediaId = mediaId;
    change.field = field;
    change.previousValue = previousValue;
    change.newValue = newValue;
    change.createdAt = QDateTime::currentDateTimeUtc();
    change.localUpdatedAt = change.createdAt;
    return true;
}
