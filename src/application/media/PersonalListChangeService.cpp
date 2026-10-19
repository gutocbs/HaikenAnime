#include "PersonalListChangeService.h"

#include "../anilist/AniListListStatus.h"
#include "../anilist/AniListPendingChangeFactory.h"

namespace {
bool appendChange(const int mediaId, const AniListField field, const AniListFieldValue &previous,
                  const AniListFieldValue &updated, QList<AniListPendingChange> &changes,
                  QString &error) {
    if (previous == updated) return true;
    AniListPendingChange change;
    if (!AniListPendingChangeFactory::Create(mediaId, field, previous, updated, change, error)) return false;
    changes.append(std::move(change));
    return true;
}
}

PersonalListChangeService::PersonalListChangeService(IPersonalListChangeWriter &writer)
    : writer_(writer) {
}

void PersonalListChangeService::setPendingChangesNotifier(std::function<void()> notifier) {
    pendingChangesNotifier_ = std::move(notifier);
}

void PersonalListChangeService::setOwnerProvider(std::function<qint64()> provider) {
    ownerProvider_ = std::move(provider);
}

bool PersonalListChangeService::save(const Media &previous, const Media &updated, QString &error) const {
    error.clear();
    if (updated.Id <= 0) {
        error = QStringLiteral("A personal-list edit requires a valid media identifier.");
        return false;
    }

    QList<AniListPendingChange> changes;
    if (!appendChange(updated.Id, AniListField::Progress, previous.ConsumedChapters,
                      updated.ConsumedChapters, changes, error)
        || !appendChange(updated.Id, AniListField::PersonalScore, previous.PersonalScore,
                         updated.PersonalScore, changes, error)
        || !appendChange(updated.Id, AniListField::ListStatus, AniListListStatusName(previous.ListStatus),
                         AniListListStatusName(updated.ListStatus), changes, error)) {
        return false;
    }
    const qint64 ownerUserId = ownerProvider_ ? ownerProvider_() : 0;
    for (auto &change : changes) change.ownerUserId = ownerUserId;
    if (!writer_.save(updated, changes, error)) return false;
    if (!changes.isEmpty() && pendingChangesNotifier_) pendingChangesNotifier_();
    return true;
}

bool PersonalListChangeService::remove(const Media &media, QString &error) const {
    error.clear();
    if (media.Id <= 0) {
        error = QStringLiteral("A personal-list removal requires a valid media identifier.");
        return false;
    }

    AniListPendingChange deletion;
    if (!AniListPendingChangeFactory::Create(media.Id, AniListField::Deletion,
                                             AniListListStatusName(media.ListStatus), QString(), deletion, error)) {
        return false;
    }
    // The user confirmed the removal in the UI, so it is sent without a further confirmation step.
    deletion.status = AniListPendingChangeStatus::Pending;
    deletion.ownerUserId = ownerProvider_ ? ownerProvider_() : 0;
    if (!writer_.remove(media, deletion, error)) return false;
    if (pendingChangesNotifier_) pendingChangesNotifier_();
    return true;
}
