#include "RemoveAniListAccountUseCase.h"

#include <QTimer>

#include <utility>

RemoveAniListAccountUseCase::RemoveAniListAccountUseCase(std::function<void()> stopOAuthListener,
                                                         SuspendCredentials suspendCredentials,
                                                         CancelPartition cancelPartition,
                                                         AniListAuthManager &authManager,
                                                         RememberLastAccount rememberLastAccount,
                                                         AuditLogger auditLogger, const int cancelTimeoutMs)
    : stopOAuthListener_(std::move(stopOAuthListener)), suspendCredentials_(std::move(suspendCredentials)),
      cancelTimeoutMs_(cancelTimeoutMs), cancelPartition_(std::move(cancelPartition)),
      authManager_(authManager), rememberLastAccount_(std::move(rememberLastAccount)),
      auditLogger_(std::move(auditLogger)) {}

RemoveAniListAccountUseCase::~RemoveAniListAccountUseCase() {
    if (run_) run_->owner = nullptr;
}

bool RemoveAniListAccountUseCase::IsRunning() const { return run_ != nullptr; }

bool RemoveAniListAccountUseCase::Start(Completion completion) {
    if (run_) return false;

    run_ = std::make_shared<Run>();
    run_->owner = this;
    run_->completion = std::move(completion);
    run_->removedUserId = authManager_.credentials().userId;
    run_->pendingCancellations = 2;
    Audit(QStringLiteral("AniList account removal started."));

    if (stopOAuthListener_) stopOAuthListener_();
    // From here on no newly started task can read the token.
    if (suspendCredentials_) suspendCredentials_(true);

    // Acknowledgements may arrive later (or re-entrantly). They hold only a weak reference to this run,
    // so a stale acknowledgement after completion (or after destruction) is ignored.
    const std::weak_ptr<Run> weakRun = run_;
    QTimer::singleShot(cancelTimeoutMs_, [weakRun] {
        const auto run = weakRun.lock();
        if (!run || !run->owner) return;
        run->owner->Audit(QStringLiteral("AniList account removal: cancel acknowledgement timed out."));
        run->pendingCancellations = 0;
        run->owner->ClearAccount();
    });
    for (const auto partition : {SyncPartition::UserList, SyncPartition::PendingChanges}) {
        auto acknowledged = [weakRun] {
            const auto run = weakRun.lock();
            if (run && run->owner) run->owner->OnCancellationAcknowledged();
        };
        if (cancelPartition_) cancelPartition_(partition, std::move(acknowledged));
        else acknowledged();
        if (weakRun.expired()) break;
    }
    return true;
}

void RemoveAniListAccountUseCase::OnCancellationAcknowledged() {
    if (--run_->pendingCancellations > 0) return;
    ClearAccount();
}

void RemoveAniListAccountUseCase::ClearAccount() {
    QString error;
    const bool cleared = authManager_.clear(error);
    // Cleared: nothing left to serve, so resuming is harmless (and lets a later reconnect work).
    // Failed: resuming keeps the still-connected account fully working.
    if (suspendCredentials_) suspendCredentials_(false);
    if (!cleared) {
        Finish(false, error);
        return;
    }
    if (run_->removedUserId > 0 && rememberLastAccount_ && !rememberLastAccount_(run_->removedUserId, error)) {
        // Credentials are already gone; the removal itself succeeded, only the offline ownership hint failed.
        Audit(QStringLiteral("AniList account removal could not remember the removed account."));
    }
    Finish(true, {});
}

void RemoveAniListAccountUseCase::Finish(const bool succeeded, const QString &error) {
    Audit(succeeded ? QStringLiteral("AniList account removal completed.")
                    : QStringLiteral("AniList account removal failed."));
    auto completion = std::move(run_->completion);
    run_.reset();
    if (completion) completion(succeeded, error);
}

void RemoveAniListAccountUseCase::Audit(const QString &message) const {
    if (auditLogger_) auditLogger_(message);
}
