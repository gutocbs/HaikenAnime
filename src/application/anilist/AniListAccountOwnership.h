#ifndef HAIKENANIME_ANILISTACCOUNTOWNERSHIP_H
#define HAIKENANIME_ANILISTACCOUNTOWNERSHIP_H

#include "AniListCredentials.h"
#include "../scheduling/SyncTaskTypes.h"

#include <QtGlobal>

/** Decisions about which AniList account owns local edits and pending changes. */
namespace AniListAccountOwnership {

/** Returns the account that owns an edit made now: the connected one, else the last connected. */
[[nodiscard]] inline qint64 ResolveOwnerUserId(const qint64 connectedUserId, const qint64 lastUserId) {
    return connectedUserId > 0 ? connectedUserId : lastUserId;
}

/** Returns the account id to remember as "last connected", or 0 when nothing must be stored. */
[[nodiscard]] inline qint64 AccountToRemember(const qint64 connectedUserId, const qint64 lastUserId) {
    return connectedUserId > 0 && connectedUserId != lastUserId ? connectedUserId : 0;
}

/** Pending changes are only sent or adopted for a validated account (userId > 0). */
[[nodiscard]] inline bool CanProcessPendingChanges(const qint64 userId) {
    return userId > 0;
}

/** The user-list sync only runs for a connected, validated account (token and userId > 0). */
[[nodiscard]] inline bool CanSynchronizeUserList(const AniListCredentials &credentials) {
    return !credentials.token.isEmpty() && credentials.userId > 0;
}

/**
 * True when a partition has nothing to do for these credentials (no account, suspended during removal, or token not
 * validated yet). The task then finishes as a silent success instead of failing with an authentication error.
 */
[[nodiscard]] inline bool ShouldSkipAniListPartition(const SyncPartition partition,
                                                     const AniListCredentials &credentials) {
    if (partition == SyncPartition::UserList) return !CanSynchronizeUserList(credentials);
    if (partition == SyncPartition::PendingChanges) {
        return credentials.token.isEmpty() || !CanProcessPendingChanges(credentials.userId);
    }
    return false;
}

}

#endif // HAIKENANIME_ANILISTACCOUNTOWNERSHIP_H
