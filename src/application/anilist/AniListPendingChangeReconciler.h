#ifndef HAIKENANIME_ANILISTPENDINGCHANGERECONCILER_H
#define HAIKENANIME_ANILISTPENDINGCHANGERECONCILER_H

#include <QList>

#include <functional>

#include "IPendingChangeRepository.h"
#include "../../domain/media/Media.h"

/** Reconciles freshly read AniList values with durable local changes before persistence. */
class AniListPendingChangeReconciler final {
public:
    using AuditLogger = std::function<void(const QString &)>;

    explicit AniListPendingChangeReconciler(IPendingChangeRepository &repository,
                                            AuditLogger auditLogger = {});

    /** Mutates remote media to the merged value and retires changes already satisfied remotely. */
    [[nodiscard]] bool reconcile(QList<Media> &media, QString &error);

private:
    IPendingChangeRepository &repository_;
    AuditLogger auditLogger_;
};

#endif // HAIKENANIME_ANILISTPENDINGCHANGERECONCILER_H
