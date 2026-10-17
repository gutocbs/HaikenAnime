#ifndef HAIKENANIME_ANILISTPENDINGCHANGEPROCESSOR_H
#define HAIKENANIME_ANILISTPENDINGCHANGEPROCESSOR_H

#include <functional>

#include "IAniListUpdateClient.h"
#include "IPendingChangeRepository.h"

/** Processes durable local changes and acknowledges them only after API confirmation. */
class AniListPendingChangeProcessor final {
public:
    /** Creates a processor with the durable queue and update client. */
    AniListPendingChangeProcessor(IPendingChangeRepository &repository,
                                  IAniListUpdateClient &updateClient, qint64 userId = 0,
                                  std::function<void(const QString &)> auditLogger = {});

    /**
     * Processes sendable changes for mediaId; confirmation-gated deletions remain queued.
     * A sendable deletion replaces every other open change of the same media.
     */
    [[nodiscard]] bool process(int mediaId, QString &error);

private:
    void log(const QString &message) const;
    [[nodiscard]] bool processDeletion(const AniListPendingChange &deletion,
                                       const QList<AniListPendingChange> &changes,
                                       QString &error);

    IPendingChangeRepository &repository_;
    IAniListUpdateClient &updateClient_;
    qint64 userId_;
    std::function<void(const QString &)> auditLogger_;
};

#endif // HAIKENANIME_ANILISTPENDINGCHANGEPROCESSOR_H
