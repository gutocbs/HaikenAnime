#include "AniListPendingChangeProcessor.h"

#include <variant>

AniListPendingChangeProcessor::AniListPendingChangeProcessor(
    IPendingChangeRepository &repository, IAniListUpdateClient &updateClient)
    : repository_(repository), updateClient_(updateClient) {
}

bool AniListPendingChangeProcessor::Send(const AniListPendingChange &change, QString &error) const {
    switch (change.field) {
    case AniListField::Progress:
        if (!std::holds_alternative<int>(change.newValue)) {
            error = QStringLiteral("Progress change must contain an integer value.");
            return false;
        }
        return updateClient_.UpdateProgress(change.mediaId, std::get<int>(change.newValue), error);
    case AniListField::PersonalScore:
        if (!std::holds_alternative<int>(change.newValue)) {
            error = QStringLiteral("Score change must contain an integer value.");
            return false;
        }
        return updateClient_.UpdateScore(change.mediaId, std::get<int>(change.newValue), error);
    case AniListField::ListStatus:
        if (!std::holds_alternative<QString>(change.newValue)) {
            error = QStringLiteral("List status change must contain a text value.");
            return false;
        }
        return updateClient_.UpdateListStatus(change.mediaId,
                                              std::get<QString>(change.newValue), error);
    case AniListField::Deletion:
        if (change.status != AniListPendingChangeStatus::RequiresConfirmation) {
            error = QStringLiteral("Deletion requires user confirmation before sending.");
            return false;
        }
        return updateClient_.DeleteListEntry(change.mediaId, error);
    default:
        error = QStringLiteral("The field does not have an update mutation.");
        return false;
    }
}

bool AniListPendingChangeProcessor::Process(const int mediaId, QString &error) {
    QList<AniListPendingChange> changes;
    if (!repository_.GetPending(mediaId, changes, error)) {
        return false;
    }

    for (auto change : changes) {
        change.status = AniListPendingChangeStatus::Processing;
        if (!repository_.UpdateStatus(change, error)) {
            return false;
        }

        QString operationError;
        if (Send(change, operationError)) {
            change.status = AniListPendingChangeStatus::Succeeded;
            change.lastError.clear();
        } else {
            change.status = AniListPendingChangeStatus::Failed;
            change.lastError = operationError;
            ++change.attempts;
        }

        QString statusError;
        if (!repository_.UpdateStatus(change, statusError)) {
            error = statusError;
            return false;
        }
        if (change.status == AniListPendingChangeStatus::Failed) {
            error = operationError;
            return false;
        }
    }

    return true;
}
