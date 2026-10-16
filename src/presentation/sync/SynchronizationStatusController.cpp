#include "SynchronizationStatusController.h"

#include <QtGlobal>

SynchronizationStatusController::SynchronizationStatusController(const int hideDelayMs, QObject *parent)
    : QObject(parent) {
    hideTimer_.setSingleShot(true);
    hideTimer_.setInterval(qMax(0, hideDelayMs));
    connect(&hideTimer_, &QTimer::timeout, this, &SynchronizationStatusController::hide);
}

bool SynchronizationStatusController::running() const { return !activePartitions_.isEmpty(); }
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

void SynchronizationStatusController::HandleStarted(const SyncPartition partition) {
    hideTimer_.stop();
    if (activePartitions_.isEmpty()) {
        roundFailed_ = false;
        failureMessage_.clear();
        stageKey_.clear();
        stageIndex_ = 0;
        stageCount_ = 0;
        processedItems_ = 0;
        totalItems_ = -1;
    }
    activePartitions_.insert(static_cast<int>(partition));
    visible_ = true;
    outcome_ = QStringLiteral("running");
    message_ = tr("Sincronizando…");
    emit changed();
}

void SynchronizationStatusController::HandleProgress(const SyncProgress &progress) {
    if (!activePartitions_.contains(static_cast<int>(progress.partition))) return;
    stageKey_ = progress.stageKey;
    stageIndex_ = progress.stageIndex;
    stageCount_ = progress.stageCount;
    processedItems_ = progress.processedItems;
    totalItems_ = progress.totalItems;
    emit changed();
}

void SynchronizationStatusController::HandleCompleted(const SyncPartition partition) {
    if (!activePartitions_.remove(static_cast<int>(partition))) return;
    finishRoundIfIdle();
    emit changed();
}

void SynchronizationStatusController::HandleFailed(const SyncPartition partition, const QString &safeError) {
    activePartitions_.remove(static_cast<int>(partition));
    roundFailed_ = true;
    failureMessage_ = safeError.isEmpty() ? tr("A sincronização falhou.") : safeError;
    visible_ = true;
    finishRoundIfIdle();
    if (!activePartitions_.isEmpty()) message_ = failureMessage_;
    emit changed();
}

void SynchronizationStatusController::finishRoundIfIdle() {
    if (!activePartitions_.isEmpty()) return;
    outcome_ = roundFailed_ ? QStringLiteral("failed") : QStringLiteral("succeeded");
    message_ = roundFailed_ ? failureMessage_ : tr("Sincronização concluída.");
    hideTimer_.start();
}

void SynchronizationStatusController::hide() {
    visible_ = false;
    outcome_ = QStringLiteral("idle");
    message_.clear();
    emit changed();
}
