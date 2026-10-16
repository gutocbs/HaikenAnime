#include "AniListPendingChangeProcessor.h"
#include "AniListPendingChangeCompactor.h"
#include "AniListPendingChangeGrouper.h"

#include <utility>

AniListPendingChangeProcessor::AniListPendingChangeProcessor(
    IPendingChangeRepository &repository, IAniListUpdateClient &updateClient, const qint64 userId)
    : repository_(repository), updateClient_(updateClient), userId_(userId) {
}

bool AniListPendingChangeProcessor::processDeletion(const AniListPendingChange &deletion,
                                                    const QList<AniListPendingChange> &changes,
                                                    QString &error) {
    auto processing = deletion;
    processing.status = AniListPendingChangeStatus::Processing;
    if (!repository_.updateStatus(processing, error)) {
        return false;
    }

    QString operationError;
    if (!updateClient_.deleteMediaListEntry(deletion.mediaId, userId_, operationError)) {
        processing.status = AniListPendingChangeStatus::Failed;
        processing.lastError = operationError;
        ++processing.attempts;
        QString statusError;
        if (!repository_.updateStatus(processing, statusError)) {
            error = statusError;
            return false;
        }
        error = operationError;
        return false;
    }

    processing.status = AniListPendingChangeStatus::Succeeded;
    processing.lastError.clear();
    if (!repository_.updateStatus(processing, error)) {
        return false;
    }
    for (auto other : changes) {
        // Includes rows the compactor only marked Superseded in memory: they must be
        // persisted too, otherwise a later drain would resend them and recreate the entry.
        if (other.id == deletion.id || other.status == AniListPendingChangeStatus::Succeeded) {
            continue;
        }
        other.status = AniListPendingChangeStatus::Superseded;
        if (!repository_.updateStatus(other, error)) {
            return false;
        }
    }
    return true;
}

bool AniListPendingChangeProcessor::process(const int mediaId, QString &error) {
    error.clear();
    QList<AniListPendingChange> changes;
    if (!repository_.getPending(mediaId, changes, error)) {
        return false;
    }

    changes = AniListPendingChangeCompactor::Compact(std::move(changes));
    for (const auto &change : changes) {
        if (change.field == AniListField::Deletion
            && (change.status == AniListPendingChangeStatus::Pending
                || change.status == AniListPendingChangeStatus::Failed
                // Left in Processing by a crash; resending is idempotent.
                || change.status == AniListPendingChangeStatus::Processing)) {
            return processDeletion(change, changes, error);
        }
    }
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
            // Only unconfirmed deletions reach this point; park them until the user confirms.
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
