#ifndef HAIKENANIME_IPENDINGCHANGEREPOSITORY_H
#define HAIKENANIME_IPENDINGCHANGEREPOSITORY_H

#include <QList>
#include <QString>

#include "../../domain/anilist/AniListPendingChange.h"

/** Persists local changes that still need to be reconciled with AniList. */
class IPendingChangeRepository {
public:
    virtual ~IPendingChangeRepository() = default;

    /** Adds a pending change to durable storage. */
    [[nodiscard]] virtual bool Enqueue(const AniListPendingChange &change, QString &error) = 0;

    /** Returns pending changes for one media item in creation order. */
    [[nodiscard]] virtual bool GetPending(int mediaId, QList<AniListPendingChange> &changes,
                                          QString &error) = 0;

    /** Updates the lifecycle state and last error of a pending change. */
    [[nodiscard]] virtual bool UpdateStatus(const AniListPendingChange &change,
                                             QString &error) = 0;
};

#endif // HAIKENANIME_IPENDINGCHANGEREPOSITORY_H
