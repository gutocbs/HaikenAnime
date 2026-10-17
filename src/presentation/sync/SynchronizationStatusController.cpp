#include "SynchronizationStatusController.h"

#include <QtGlobal>

SynchronizationStatusController::SynchronizationStatusController(const int hideDelayMs, const int settleDelayMs,
                                                                 QObject *parent)
    : QObject(parent) {
    hideTimer_.setSingleShot(true);
    hideTimer_.setInterval(qMax(0, hideDelayMs));
    connect(&hideTimer_, &QTimer::timeout, this, &SynchronizationStatusController::hide);
    settleTimer_.setSingleShot(true);
    settleTimer_.setInterval(qMax(0, settleDelayMs));
    connect(&settleTimer_, &QTimer::timeout, this, &SynchronizationStatusController::publishOutcome);
}

bool SynchronizationStatusController::isTracked(const SyncPartition partition) {
    return partition == SyncPartition::UserList || partition == SyncPartition::PendingChanges;
}

bool SynchronizationStatusController::running() const { return roundActive_; }
QString SynchronizationStatusController::outcome() const { return outcome_; }
QString SynchronizationStatusController::stageKey() const { return stageKey_; }

QString SynchronizationStatusController::stageLabel() const {
    if (stageKey_ == QLatin1String("anime")) return tr("Anime");
    if (stageKey_ == QLatin1String("manga")) return tr("Manga");
    if (stageKey_ == QLatin1String("reconciliation")) return tr("Reconciliação");
    if (stageKey_ == QLatin1String("pending_changes")) return tr("Alterações pendentes");
    return {};
}

int SynchronizationStatusController::stageIndex() const { return stageIndex_; }
int SynchronizationStatusController::stageCount() const { return stageCount_; }
int SynchronizationStatusController::processedItems() const { return processedItems_; }
int SynchronizationStatusController::totalItems() const { return totalItems_; }
bool SynchronizationStatusController::progressKnown() const { return totalItems_ > 0; }

int SynchronizationStatusController::percent() const {
    if (totalItems_ <= 0) return 0;
    return qBound(0, static_cast<int>(static_cast<qint64>(processedItems_) * 100 / totalItems_), 100);
}

QString SynchronizationStatusController::message() const { return message_; }
bool SynchronizationStatusController::visible() const { return visible_; }

void SynchronizationStatusController::beginRoundIfSettled() {
    // Only the settle window continues a round. Once an outcome was published (or the bar is hidden) the
    // next task starts a new round, so an earlier failure never taints a later successful retry.
    if (roundActive_) return;
    // Bookkeeping starts at once, but a lingering outcome stays on screen until it expires or this round shows
    // progress or fails.
    roundShown_ = false;
    roundFailed_ = false;
    roundHadOutcome_ = false;
    failureMessage_.clear();
    stageKey_.clear();
    stageIndex_ = 0;
    stageCount_ = 0;
    processedItems_ = 0;
    totalItems_ = -1;
}

void SynchronizationStatusController::HandleStarted(const SyncPartition partition) {
    if (!isTracked(partition)) return;
    settleTimer_.stop();
    beginRoundIfSettled();
    roundActive_ = true;
    activePartitions_.insert(static_cast<int>(partition));
    if (!visible_) {
        outcome_ = QStringLiteral("running");
        message_ = tr("Sincronizando…");
    }
    emit changed();
}

void SynchronizationStatusController::HandleProgress(const SyncProgress &progress) {
    if (!activePartitions_.contains(static_cast<int>(progress.partition))) return;
    stageKey_ = progress.stageKey;
    stageIndex_ = progress.stageIndex;
    stageCount_ = progress.stageCount;
    processedItems_ = progress.processedItems;
    totalItems_ = progress.totalItems;
    // The bar only appears once real work is reported.
    roundShown_ = true;
    hideTimer_.stop();
    visible_ = true;
    outcome_ = QStringLiteral("running");
    message_ = tr("Sincronizando…");
    emit changed();
}

void SynchronizationStatusController::HandleCompleted(const SyncPartition partition) {
    if (!activePartitions_.remove(static_cast<int>(partition))) return;
    roundHadOutcome_ = true;
    settleIfIdle();
    emit changed();
}

void SynchronizationStatusController::HandleCancelled(const SyncPartition partition) {
    if (!activePartitions_.remove(static_cast<int>(partition))) return;
    // A cancellation is neither a success nor a failure: when nothing else happened in the round, the bar
    // simply disappears instead of announcing an outcome.
    if (activePartitions_.isEmpty() && !roundHadOutcome_) {
        settleTimer_.stop();
        roundActive_ = false;
        if (roundShown_) {
            hideTimer_.stop();
            hide();
        } else {
            finishSilently();
        }
        return;
    }
    settleIfIdle();
    emit changed();
}

void SynchronizationStatusController::HandleFailed(const SyncPartition partition, const QString &safeError) {
    if (!isTracked(partition)) return;
    hideTimer_.stop();
    beginRoundIfSettled();
    activePartitions_.remove(static_cast<int>(partition));
    roundActive_ = true;
    roundShown_ = true;
    roundFailed_ = true;
    roundHadOutcome_ = true;
    failureMessage_ = safeError.isEmpty() ? tr("A sincronização falhou.") : safeError;
    visible_ = true;
    outcome_ = QStringLiteral("running");
    message_ = tr("Sincronizando…");
    settleIfIdle();
    emit changed();
}

void SynchronizationStatusController::HandleInitializationFailed(const QString &safeError) {
    HandleFailed(SyncPartition::UserList, safeError);
}

void SynchronizationStatusController::settleIfIdle() {
    if (activePartitions_.isEmpty()) settleTimer_.start();
}

void SynchronizationStatusController::publishOutcome() {
    if (!activePartitions_.isEmpty()) return;
    roundActive_ = false;
    if (!roundShown_ && !roundFailed_) {
        // Nothing was shown (no progress, no failure): finish silently without a success flash.
        finishSilently();
        return;
    }
    outcome_ = roundFailed_ ? QStringLiteral("failed") : QStringLiteral("succeeded");
    message_ = roundFailed_ ? failureMessage_ : tr("Sincronização concluída.");
    hideTimer_.start();
    emit changed();
}

void SynchronizationStatusController::finishSilently() {
    // Leave any lingering outcome of an earlier round untouched; only drop the internal "running" marker.
    if (!visible_) {
        outcome_ = QStringLiteral("idle");
        message_.clear();
    }
    emit changed();
}

void SynchronizationStatusController::hide() {
    visible_ = false;
    outcome_ = QStringLiteral("idle");
    message_.clear();
    emit changed();
}
