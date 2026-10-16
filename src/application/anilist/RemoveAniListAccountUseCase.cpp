#include "RemoveAniListAccountUseCase.h"

#include <utility>

RemoveAniListAccountUseCase::RemoveAniListAccountUseCase(std::function<void()> stopOAuthListener,
                                                         CancelPartition cancelPartition,
                                                         AniListAuthManager &authManager,
                                                         RememberLastAccount rememberLastAccount,
                                                         AuditLogger auditLogger)
    : stopOAuthListener_(std::move(stopOAuthListener)), cancelPartition_(std::move(cancelPartition)),
      authManager_(authManager), rememberLastAccount_(std::move(rememberLastAccount)),
      auditLogger_(std::move(auditLogger)) {}

bool RemoveAniListAccountUseCase::IsRunning() const { return run_ != nullptr; }

bool RemoveAniListAccountUseCase::Start(Completion completion) {
    if (run_) return false;

    run_ = std::make_shared<Run>();
    run_->completion = std::move(completion);
    run_->removedUserId = authManager_.credentials().userId;
    run_->pendingCancellations = 2;
    Audit(QStringLiteral("AniList account removal started."));

    if (stopOAuthListener_) stopOAuthListener_();
    // Acknowledgements may arrive later (or re-entrantly); they only count down on this run, and a
    // stale acknowledgement after completion is ignored through the weak reference.
    const std::weak_ptr<Run> weakRun = run_;
    for (const auto partition : {SyncPartition::UserList, SyncPartition::PendingChanges}) {
        auto acknowledged = [this, weakRun] {
            if (weakRun.lock() == run_ && run_) OnCancellationAcknowledged();
        };
        if (cancelPartition_) cancelPartition_(partition, std::move(acknowledged));
        else acknowledged();
        if (!run_ || weakRun.expired()) break;
    }
    return true;
}

void RemoveAniListAccountUseCase::OnCancellationAcknowledged() {
    if (--run_->pendingCancellations > 0) return;

    QString error;
    if (!authManager_.clear(error)) {
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
