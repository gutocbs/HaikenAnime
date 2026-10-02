#include "AniListPendingChangeFactory.h"

bool AniListPendingChangeFactory::Create(const int mediaId, const AniListField field,
                                         const AniListFieldValue &previousValue,
                                         const AniListFieldValue &newValue,
                                         AniListPendingChange &change, QString &error) {
    change = {};
    error.clear();

    if (mediaId <= 0) {
        error = QStringLiteral("A pending AniList change requires a positive media identifier.");
        return false;
    }

    const bool expectsInteger = field == AniListField::Progress
        || field == AniListField::PersonalScore;
    const bool expectsText = field == AniListField::ListStatus
        || field == AniListField::Deletion;
    if ((!expectsInteger && !expectsText)
        || (expectsInteger && (!std::holds_alternative<int>(previousValue)
                               || !std::holds_alternative<int>(newValue)))
        || (expectsText && (!std::holds_alternative<QString>(previousValue)
                            || !std::holds_alternative<QString>(newValue)))) {
        error = QStringLiteral("The field is not writable or its pending value has an invalid type.");
        return false;
    }

    if (field != AniListField::Deletion && previousValue == newValue) {
        error = QStringLiteral("A pending AniList change requires different values.");
        return false;
    }

    change.mediaId = mediaId;
    change.field = field;
    change.previousValue = previousValue;
    change.newValue = newValue;
    change.createdAt = QDateTime::currentDateTimeUtc();
    change.localUpdatedAt = change.createdAt;
    if (field == AniListField::Deletion) {
        change.status = AniListPendingChangeStatus::RequiresConfirmation;
    }
    return true;
}
