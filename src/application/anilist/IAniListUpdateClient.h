#ifndef HAIKENANIME_IANILISTUPDATECLIENT_H
#define HAIKENANIME_IANILISTUPDATECLIENT_H

#include <QtGlobal>
#include <QString>

#include "AniListMediaPendingChanges.h"

/** Sends user-originated changes to AniList. */
class IAniListUpdateClient {
public:
    virtual ~IAniListUpdateClient() = default;

    /** Sends all active changes for one media item and clears error on success. */
    [[nodiscard]] virtual bool updateMedia(const AniListMediaPendingChanges &changes,
                                           QString &error) = 0;

    /**
     * Removes the user's list entry for mediaId from AniList and clears error on success.
     * An entry that no longer exists remotely counts as success.
     */
    [[nodiscard]] virtual bool deleteMediaListEntry(int mediaId, qint64 userId,
                                                    QString &error) = 0;
};

#endif // HAIKENANIME_IANILISTUPDATECLIENT_H
