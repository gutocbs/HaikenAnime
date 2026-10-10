#ifndef HAIKENANIME_ANILISTPENDINGCHANGEDRAIN_H
#define HAIKENANIME_ANILISTPENDINGCHANGEDRAIN_H

#include <functional>

#include "AniListPendingChangeProcessor.h"

/** Drains the AniList outbox, re-reading durable work until no sendable changes remain. */
class AniListPendingChangeDrain final {
public:
    using CancellationProbe = std::function<bool()>;

    AniListPendingChangeDrain(IPendingChangeRepository &repository,
                              AniListPendingChangeProcessor &processor);

    [[nodiscard]] bool processAll(QString &error,
                                  const CancellationProbe &isCancelled = {});

private:
    IPendingChangeRepository &repository_;
    AniListPendingChangeProcessor &processor_;
};

#endif // HAIKENANIME_ANILISTPENDINGCHANGEDRAIN_H
