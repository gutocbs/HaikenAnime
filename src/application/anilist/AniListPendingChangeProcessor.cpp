#include "AniListPendingChangeProcessor.h"
#include "AniListPendingChangeCompactor.h"
#include "AniListPendingChangeGrouper.h"

#include <utility>

AniListPendingChangeProcessor::AniListPendingChangeProcessor(
    IPendingChangeRepository &repository, IAniListUpdateClient &updateClient)
    : repository_(repository), updateClient_(updateClient) {
}

bool AniListPendingChangeProcessor::Process(const int mediaId, QString &error) {
    QList<AniListPendingChange> changes;
    if (!repository_.GetPending(mediaId, changes, error)) {
        return false;
    }

    changes = AniListPendingChangeCompactor::Compact(std::move(changes));
    QList<AniListPendingChange> activeChanges;
    for (const auto &change : changes) {
        if (change.status == AniListPendingChangeStatus::Superseded) {
            QString statusError;
            if (!repository_.UpdateStatus(change, statusError)) {
                error = statusError;
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
            if (!repository_.UpdateStatus(change, error)) {
                return false;
            }
            processingChanges.append(change);
        }

        QString operationError;
        if (updateClient_.UpdateMedia(group, operationError)) {
            for (auto change : processingChanges) {
            change.status = AniListPendingChangeStatus::Succeeded;
            change.lastError.clear();
                QString statusError;
                if (!repository_.UpdateStatus(change, statusError)) {
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
                if (!repository_.UpdateStatus(change, statusError)) {
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
