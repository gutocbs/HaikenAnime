#ifndef HAIKENANIME_IANILISTPENDINGCHANGEOBSERVER_H
#define HAIKENANIME_IANILISTPENDINGCHANGEOBSERVER_H

#include <QList>

#include "AniListPendingChange.h"
#include "AniListSyncErrorCategory.h"

/**
 * Learns the outcome of outbox changes after the processor has persisted it.
 * Observers must not fail the drain: they cannot alter outbox statuses or the processing result.
 * Changes are passed without lastError; failures carry only the classified category, never the
 * raw (possibly credential-bearing) error message.
 */
class IAniListPendingChangeObserver {
public:
    virtual ~IAniListPendingChangeObserver() = default;

    /** Changes confirmed by AniList and already acknowledged as Succeeded in the outbox. */
    virtual void changesSent(const QList<AniListPendingChange> &changes) = 0;
    /** Changes kept Failed in the outbox (resent by a later drain); category tells whether retrying may help. */
    virtual void changesFailed(const QList<AniListPendingChange> &changes, AniListSyncErrorCategory category) = 0;
};

#endif // HAIKENANIME_IANILISTPENDINGCHANGEOBSERVER_H
