#ifndef HAIKENANIME_IANILISTUPDATECLIENT_H
#define HAIKENANIME_IANILISTUPDATECLIENT_H

#include <QString>

#include "AniListMediaPendingChanges.h"

/** Sends user-originated changes to AniList. */
class IAniListUpdateClient {
public:
    virtual ~IAniListUpdateClient() = default;

    /** Sends all active changes for one media item and clears error on success. */
    [[nodiscard]] virtual bool updateMedia(const AniListMediaPendingChanges &changes,
                                           QString &error) = 0;

};

#endif // HAIKENANIME_IANILISTUPDATECLIENT_H
