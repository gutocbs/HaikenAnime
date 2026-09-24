#ifndef HAIKENANIME_ANILISTPENDINGCHANGEPROCESSOR_H
#define HAIKENANIME_ANILISTPENDINGCHANGEPROCESSOR_H

#include "IAniListUpdateClient.h"
#include "IPendingChangeRepository.h"

/** Processes durable local changes and acknowledges them only after API confirmation. */
class AniListPendingChangeProcessor final {
public:
    /** Creates a processor with the durable queue and update client. */
    AniListPendingChangeProcessor(IPendingChangeRepository &repository,
                                  IAniListUpdateClient &updateClient);

    /** Processes sendable changes for mediaId; confirmation-gated deletions remain queued. */
    [[nodiscard]] bool process(int mediaId, QString &error);

private:
    IPendingChangeRepository &repository_;
    IAniListUpdateClient &updateClient_;
};

#endif // HAIKENANIME_ANILISTPENDINGCHANGEPROCESSOR_H
