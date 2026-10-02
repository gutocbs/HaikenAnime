#include "AniListPendingChangeProcessor.h"
#include "AniListPendingChangeCompactor.h"
#include "AniListPendingChangeGrouper.h"

#include <utility>

AniListPendingChangeProcessor::AniListPendingChangeProcessor(
    IPendingChangeRepository &repository, IAniListUpdateClient &updateClient)
    : repository_(repository), updateClient_(updateClient) {
}

bool AniListPendingChangeProcessor::process(const int mediaId, QString &error) {
    error.clear();
    QList<AniListPendingChange> changes;
    if (!repository_.getPending(mediaId, changes, error)) {
        return false;
    }

    changes = AniListPendingChangeCompactor::Compact(std::move(changes));
    QList<AniListPendingChange> activeChanges;
    for (auto change : changes) {
        if (change.status == AniListPendingChangeStatus::Superseded) {
            QString statusError;
            if (!repository_.updateStatus(change, statusError)) {
                error = statusError;
                return false;
            }
            continue;
        }
        if (change.status == AniListPendingChangeStatus::RequiresConfirmation) {
            continue;
        }
        if (change.field == AniListField::Deletion) {
            change.status = AniListPendingChangeStatus::RequiresConfirmation;
            if (!repository_.updateStatus(change, error)) {
                return false;
            }
            continue;
        }
        activeChanges.append(change);
    }

    const auto groups = AniListPendingChangeGrouper::Group(std::move(activeChanges));
    for (const auto &group : groups) {
        QList<AniListPendingChange> processingChanges;
        for (auto change : group.changes) {
            change.status = AniListPendingChangeStatus::Processing;
            if (!repository_.updateStatus(change, error)) {
                return false;
            }
            processingChanges.append(change);
        }

        QString operationError;
        if (updateClient_.updateMedia(group, operationError)) {
            for (auto change : processingChanges) {
                change.status = AniListPendingChangeStatus::Succeeded;
                change.lastError.clear();
                QString statusError;
                if (!repository_.updateStatus(change, statusError)) {
                    error = statusError;
                    return false;
                }
            }
        } else {
            for (auto change : processingChanges) {
                change.status = AniListPendingChangeStatus::Failed;
                change.lastError = operationError;
                ++change.attempts;
                QString statusError;
                if (!repository_.updateStatus(change, statusError)) {
                    error = statusError;
                    return false;
                }
            }
            error = operationError;
            return false;
        }
    }

    return true;
}
