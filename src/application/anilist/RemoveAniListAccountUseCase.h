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
    /** Called with true before cancelling (credentials must stop being served) and false after clear(). */
    using SuspendCredentials = std::function<void(bool suspended)>;
    static constexpr int DefaultCancelTimeoutMs = 10'000;

    /**
     * Order: stop listener -> suspend credentials -> cancel UserList + PendingChanges (bounded wait of
     * `cancelTimeoutMs`; on timeout it proceeds) -> clear() -> resume (no-op for an emptied account, and
     * restores the account if clear() failed) -> remember removed account -> completion.
     */
    RemoveAniListAccountUseCase(std::function<void()> stopOAuthListener, SuspendCredentials suspendCredentials,
                                CancelPartition cancelPartition, AniListAuthManager &authManager,
                                RememberLastAccount rememberLastAccount, AuditLogger auditLogger = {},
                                int cancelTimeoutMs = DefaultCancelTimeoutMs);
    ~RemoveAniListAccountUseCase();
    RemoveAniListAccountUseCase(const RemoveAniListAccountUseCase &) = delete;
    RemoveAniListAccountUseCase &operator=(const RemoveAniListAccountUseCase &) = delete;

    /** Starts the removal; returns false when one is already running. Completion runs exactly once. */
    [[nodiscard]] bool Start(Completion completion);
    [[nodiscard]] bool IsRunning() const;

private:
    struct Run final {
        RemoveAniListAccountUseCase *owner = nullptr; // lifetime token: reset in the destructor
        Completion completion;
        qint64 removedUserId = 0;
        int pendingCancellations = 0;
    };

    void OnCancellationAcknowledged();
    void ClearAccount();
    void Finish(bool succeeded, const QString &error);
    void Audit(const QString &message) const;

    std::function<void()> stopOAuthListener_;
    SuspendCredentials suspendCredentials_;
    int cancelTimeoutMs_ = DefaultCancelTimeoutMs;
    CancelPartition cancelPartition_;
    AniListAuthManager &authManager_;
    RememberLastAccount rememberLastAccount_;
    AuditLogger auditLogger_;
    std::shared_ptr<Run> run_;
};

#endif // HAIKENANIME_REMOVEANILISTACCOUNTUSECASE_H
