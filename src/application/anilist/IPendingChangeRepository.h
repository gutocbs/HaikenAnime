#ifndef HAIKENANIME_IPENDINGCHANGEREPOSITORY_H
#define HAIKENANIME_IPENDINGCHANGEREPOSITORY_H

#include <QList>
#include <QString>

#include "AniListPendingChange.h"

/** Persists local changes that still need to be reconciled with AniList. */
class IPendingChangeRepository {
public:
    virtual ~IPendingChangeRepository() = default;

    /** Adds a pending change to durable storage and clears error on success. */
    [[nodiscard]] virtual bool enqueue(const AniListPendingChange &change, QString &error) = 0;

    /** Replaces changes with pending items in creation order and clears error on success. */
    [[nodiscard]] virtual bool getPending(int mediaId, QList<AniListPendingChange> &changes,
                                          QString &error) = 0;

    /** Updates the lifecycle state and last error of a pending change, clearing error on success. */
    [[nodiscard]] virtual bool updateStatus(const AniListPendingChange &change,
                                             QString &error) = 0;
};

#endif // HAIKENANIME_IPENDINGCHANGEREPOSITORY_H
