#ifndef HAIKENANIME_ANILISTPENDINGCHANGEPROCESSOR_H
#define HAIKENANIME_ANILISTPENDINGCHANGEPROCESSOR_H

#include <functional>

#include "IAniListPendingChangeObserver.h"
#include "IAniListUpdateClient.h"
#include "IPendingChangeRepository.h"

/** Processes durable local changes and acknowledges them only after API confirmation. */
class AniListPendingChangeProcessor final {
public:
    /** Creates a processor with the durable queue and update client. */
    AniListPendingChangeProcessor(IPendingChangeRepository &repository,
                                  IAniListUpdateClient &updateClient, qint64 userId = 0,
                                  std::function<void(const QString &)> auditLogger = {},
                                  std::function<bool()> isCancelled = {});

    /**
     * Processes sendable changes for mediaId; confirmation-gated deletions remain queued.
     * A sendable deletion replaces every other open change of the same media.
     */
    [[nodiscard]] bool process(int mediaId, QString &error);

    /** Optional, non-owning; told the persisted outcome of each sent group. Never alters the outbox. */
    void setObserver(IAniListPendingChangeObserver *observer) { observer_ = observer; }

private:
    void log(const QString &message) const;
    [[nodiscard]] bool processDeletion(const AniListPendingChange &deletion,
                                       const QList<AniListPendingChange> &changes,
                                       QString &error);

    IPendingChangeRepository &repository_;
    IAniListUpdateClient &updateClient_;
    qint64 userId_;
    std::function<void(const QString &)> auditLogger_;
    // A cancelled request is not a failed attempt: the change is restored and resent later.
    std::function<bool()> isCancelled_;
    IAniListPendingChangeObserver *observer_ = nullptr;
};

#endif // HAIKENANIME_ANILISTPENDINGCHANGEPROCESSOR_H
