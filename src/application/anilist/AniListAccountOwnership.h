#ifndef HAIKENANIME_ANILISTACCOUNTOWNERSHIP_H
#define HAIKENANIME_ANILISTACCOUNTOWNERSHIP_H

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

}

#endif // HAIKENANIME_ANILISTACCOUNTOWNERSHIP_H
