#ifndef HAIKENANIME_ANILISTPENDINGCHANGEDRAIN_H
#define HAIKENANIME_ANILISTPENDINGCHANGEDRAIN_H

#include <functional>

#include "AniListPendingChangeProcessor.h"

/** Drains the AniList outbox, re-reading durable work until no sendable changes remain. */
class AniListPendingChangeDrain final {
public:
    using CancellationProbe = std::function<bool()>;
    using ProgressReporter = std::function<void(int processed, int total)>;

    AniListPendingChangeDrain(IPendingChangeRepository &repository,
                              AniListPendingChangeProcessor &processor);

    /** Reports (processed, total) media after each media of a drain pass is processed. */
    void setProgressReporter(ProgressReporter reporter) { progressReporter_ = std::move(reporter); }

    [[nodiscard]] bool processAll(QString &error,
                                  const CancellationProbe &isCancelled = {});

private:
    IPendingChangeRepository &repository_;
    AniListPendingChangeProcessor &processor_;
    ProgressReporter progressReporter_;
};

#endif // HAIKENANIME_ANILISTPENDINGCHANGEDRAIN_H
