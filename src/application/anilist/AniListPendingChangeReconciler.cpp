#include "AniListPendingChangeReconciler.h"

#include "AniListMergeService.h"
#include "AniListPendingChangeCompactor.h"

#include <optional>
#include <utility>

namespace {
QString AniListStatus(const UserListStatus status) {
    switch (status) {
    case UserListStatus::Current: return QStringLiteral("CURRENT");
    case UserListStatus::Planning: return QStringLiteral("PLANNING");
    case UserListStatus::OnHold: return QStringLiteral("PAUSED");
    case UserListStatus::Dropped: return QStringLiteral("DROPPED");
    case UserListStatus::Completed: return QStringLiteral("COMPLETED");
    case UserListStatus::Unknown: return {};
    }
    return {};
}

std::optional<UserListStatus> UserStatus(const QString &value) {
    const auto normalized = value.trimmed().toUpper();
    if (normalized == QStringLiteral("CURRENT")) return UserListStatus::Current;
    if (normalized == QStringLiteral("PLANNING")) return UserListStatus::Planning;
    if (normalized == QStringLiteral("PAUSED")) return UserListStatus::OnHold;
    if (normalized == QStringLiteral("DROPPED")) return UserListStatus::Dropped;
    if (normalized == QStringLiteral("COMPLETED")) return UserListStatus::Completed;
    return std::nullopt;
}

bool CurrentValue(const Media &media, const AniListField field,
                  AniListFieldValue &value, QString &error) {
    switch (field) {
    case AniListField::Progress:
        value = media.ConsumedChapters;
        return true;
    case AniListField::PersonalScore:
        value = media.PersonalScore;
        return true;
    case AniListField::ListStatus:
        value = AniListStatus(media.ListStatus);
        return true;
    case AniListField::Deletion:
        return false;
    default:
        error = QStringLiteral("Pending AniList field cannot be reconciled with user-list media.");
        return false;
    }
}

bool ApplyValue(Media &media, const AniListField field,
                const AniListFieldValue &value, QString &error) {
    switch (field) {
    case AniListField::Progress:
        if (const auto *progress = std::get_if<int>(&value)) {
            media.ConsumedChapters = *progress;
            return true;
        }
        break;
    case AniListField::PersonalScore:
        if (const auto *score = std::get_if<int>(&value)) {
            media.PersonalScore = *score;
            return true;
        }
        break;
    case AniListField::ListStatus:
        if (const auto *status = std::get_if<QString>(&value)) {
            const auto parsed = UserStatus(*status);
            if (parsed.has_value()) {
                media.ListStatus = *parsed;
                return true;
            }
        }
        break;
    default:
        break;
    }
    error = QStringLiteral("Pending AniList value is incompatible with its media field.");
    return false;
}
}

AniListPendingChangeReconciler::AniListPendingChangeReconciler(
    IPendingChangeRepository &repository, AuditLogger auditLogger)
    : repository_(repository), auditLogger_(std::move(auditLogger)) {
}

bool AniListPendingChangeReconciler::reconcile(QList<Media> &media, QString &error) {
    error.clear();
    int examined = 0;
    int retired = 0;
    int preserved = 0;
    if (auditLogger_) {
        auditLogger_(QStringLiteral("AniList pending-change reconciliation started: media=%1.")
                         .arg(media.size()));
    }
    for (auto &item : media) {
        QList<AniListPendingChange> changes;
        if (!repository_.getPending(item.Id, changes, error)) return false;

        changes = AniListPendingChangeCompactor::Compact(std::move(changes));
        for (auto change : changes) {
            ++examined;
            if (change.status == AniListPendingChangeStatus::Superseded) {
                if (!repository_.updateStatus(change, error)) return false;
                ++retired;
                continue;
            }
            if (change.status == AniListPendingChangeStatus::RequiresConfirmation
                || change.field == AniListField::Deletion) {
                ++preserved;
                continue;
            }

            AniListFieldValue remoteValue;
            if (!CurrentValue(item, change.field, remoteValue, error)) return false;
            const auto decision = AniListMergeService::Merge(
                change.field, change.newValue, remoteValue);
            if (decision.result == AniListMergeResult::Conflict) {
                error = QStringLiteral("Pending AniList change conflicts with the synchronized value.");
                return false;
            }

            if (decision.value == remoteValue) {
                change.status = change.newValue == remoteValue
                    ? AniListPendingChangeStatus::Succeeded
                    : AniListPendingChangeStatus::Superseded;
                change.lastError.clear();
                if (!repository_.updateStatus(change, error)) return false;
                ++retired;
                continue;
            }
            if (!ApplyValue(item, change.field, decision.value, error)) return false;
            ++preserved;
        }
    }
    if (auditLogger_) {
        auditLogger_(QStringLiteral(
            "AniList pending-change reconciliation completed: examined=%1 retired=%2 preserved=%3.")
                         .arg(examined)
                         .arg(retired)
                         .arg(preserved));
    }
    return true;
}
