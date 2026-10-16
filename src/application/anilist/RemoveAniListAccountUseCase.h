#ifndef HAIKENANIME_REMOVEANILISTACCOUNTUSECASE_H
#define HAIKENANIME_REMOVEANILISTACCOUNTUSECASE_H

#include <QString>

#include <functional>
#include <memory>

#include "AniListAuthManager.h"
#include "../scheduling/SyncTaskTypes.h"

/**
 * Removes the connected AniList account: stops the OAuth listener, cancels the account's synchronization,
 * deletes the stored credentials and remembers the removed account as the owner of later offline edits.
 * The local library and the pending-change queue are never touched. Not thread-safe: use from one thread.
 */
class RemoveAniListAccountUseCase final {
public:
    using Completion = std::function<void(bool succeeded, QString error)>;
    using CancelPartition = std::function<void(SyncPartition, std::function<void()> acknowledged)>;
    using RememberLastAccount = std::function<bool(qint64 userId, QString &error)>;
    using AuditLogger = AniListAuthManager::AuditLogger;

    RemoveAniListAccountUseCase(std::function<void()> stopOAuthListener, CancelPartition cancelPartition,
                                AniListAuthManager &authManager, RememberLastAccount rememberLastAccount,
                                AuditLogger auditLogger = {});

    /** Starts the removal; returns false when one is already running. Completion runs exactly once. */
    [[nodiscard]] bool Start(Completion completion);
    [[nodiscard]] bool IsRunning() const;

private:
    struct Run final {
        Completion completion;
        qint64 removedUserId = 0;
        int pendingCancellations = 0;
    };

    void OnCancellationAcknowledged();
    void Finish(bool succeeded, const QString &error);
    void Audit(const QString &message) const;

    std::function<void()> stopOAuthListener_;
    CancelPartition cancelPartition_;
    AniListAuthManager &authManager_;
    RememberLastAccount rememberLastAccount_;
    AuditLogger auditLogger_;
    std::shared_ptr<Run> run_;
};

#endif // HAIKENANIME_REMOVEANILISTACCOUNTUSECASE_H
