#include "PersonalListChangeService.h"

#include "../anilist/AniListPendingChangeFactory.h"

namespace {
QString aniListStatus(const UserListStatus status) {
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
        || !appendChange(updated.Id, AniListField::ListStatus, aniListStatus(previous.ListStatus),
                         aniListStatus(updated.ListStatus), changes, error)) {
        return false;
    }
    if (!writer_.save(updated, changes, error)) return false;
    if (!changes.isEmpty() && pendingChangesNotifier_) pendingChangesNotifier_();
    return true;
}
