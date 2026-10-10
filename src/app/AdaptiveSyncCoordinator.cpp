#include "AdaptiveSyncCoordinator.h"

#include <QMetaObject>
#include <QPointer>
#include <QTimer>

#include <algorithm>
#include <limits>
#include <utility>

namespace {
QString SafeError(const QString &error, const QString &fallback) {
    return error.isEmpty() ? fallback : error;
}

SyncPartition PartitionFor(const SyncTaskKind kind) {
    switch (kind) {
    case SyncTaskKind::UserList: return SyncPartition::UserList;
    case SyncTaskKind::PendingChange: return SyncPartition::PendingChanges;
    case SyncTaskKind::ActiveCatalog: return SyncPartition::ActiveCatalog;
    case SyncTaskKind::InactiveCatalog: return SyncPartition::InactiveCatalog;
    case SyncTaskKind::CompletedCatalog: return SyncPartition::CompletedCatalog;
    case SyncTaskKind::Cover: return SyncPartition::Covers;
    case SyncTaskKind::DerivedMetadata: return SyncPartition::DerivedMetadata;
    }
    return SyncPartition::UserList;
}
}

AdaptiveSyncCoordinator::AdaptiveSyncCoordinator(
    ISyncTaskStateRepository &stateRepository, ISyncTaskExecutor &executor, Clock clock,
    std::map<SyncTaskKind, SyncSchedulePolicy> policies, const bool seedMissingTasks, QObject *parent)
    : QObject(parent), stateRepository_(stateRepository), executor_(executor), clock_(std::move(clock)),
      policies_(std::move(policies)), wakeUpTimer_(new QTimer(this)), seedMissingTasks_(seedMissingTasks) {
    wakeUpTimer_->setSingleShot(true);
    connect(wakeUpTimer_, &QTimer::timeout, this, &AdaptiveSyncCoordinator::ProcessDueTasks);
}

AdaptiveSyncCoordinator::~AdaptiveSyncCoordinator() {
    Stop();
}

bool AdaptiveSyncCoordinator::Start() {
    if (stopped_ || started_) return false;

    QList<SyncTaskState> persistedStates;
    QString error;
    if (!stateRepository_.ReadAll(persistedStates, error)) {
        emit SchedulingFailed(SafeError(error, QStringLiteral("Unable to load synchronization task state.")));
        return false;
    }

    QList<SyncPartition> restartScheduledPartitions;
    const auto now = Now();
    for (const auto &state : persistedStates) {
        if (!policies_.empty() && !policies_.contains(state.kind)) {
            continue;
        }
        states_[state.partition] = state;
        if (state.status != SyncTaskStatus::Succeeded && state.nextRunAt.has_value()
            && *state.nextRunAt > now) {
            restartScheduledPartitions.append(state.partition);
        }
    }
    if (seedMissingTasks_) {
        for (const auto &[kind, policy] : policies_) {
            Q_UNUSED(policy);
            const auto partition = PartitionFor(kind);
            if (states_.contains(partition)) continue;
            SyncTaskState state;
            state.kind = kind;
            state.partition = partition;
            state.priority = SyncTaskPolicy::PriorityFor(partition);
            if (!Persist(state)) return false;
            states_[partition] = state;
        }
    }
    started_ = true;
    ProcessDueTasks();
    for (const auto partition : restartScheduledPartitions) RequestNow(partition);
    return true;
}

void AdaptiveSyncCoordinator::Stop() {
    if (stopped_ || stopping_) return;

    stopping_ = true;
    wakeUpTimer_->stop();
    QPointer<AdaptiveSyncCoordinator> coordinator(this);
    for (const auto &[partition, generation] : activeGenerations_) {
        auto state = states_.at(partition);
        state.generation = generation + 1;
        state.status = SyncTaskStatus::Idle;
        if (!Persist(state)) continue;
        states_[partition] = state;
        executor_.Cancel(partition, [coordinator, partition, generation] {
            if (!coordinator) return;
            QMetaObject::invokeMethod(coordinator, [coordinator, partition, generation] {
                if (coordinator) coordinator->AcknowledgeCancellation(partition, generation);
            }, Qt::QueuedConnection);
        });
    }
    executor_.Shutdown([coordinator] {
        if (!coordinator) return;
        QMetaObject::invokeMethod(coordinator, [coordinator] {
            if (coordinator) coordinator->AcknowledgeShutdown();
        }, Qt::QueuedConnection);
    });
    FinishStopping();
}

void AdaptiveSyncCoordinator::RequestNow(const SyncPartition partition) {
    Request(partition, SyncTaskTrigger::ManualRun);
}

void AdaptiveSyncCoordinator::NotifyLocalChange(const SyncPartition partition) {
    Request(partition, partition == SyncPartition::CompletedCatalog
                           ? SyncTaskTrigger::CompletedToActive : SyncTaskTrigger::LocalChange);
}

void AdaptiveSyncCoordinator::ProcessDueTasks() {
    if (!started_ || stopped_) return;
    if (!activeGenerations_.empty()) {
        ScheduleWakeUp();
        return;
    }

    const auto now = Now();
    QList<SyncTaskState> due;
    for (auto &[partition, state] : states_) {
        if (activeGenerations_.contains(partition)) continue;
        if (state.nextRunAt.has_value()) {
            if (*state.nextRunAt <= now && CanStart(partition)) due.append(state);
            continue;
        }
        const auto decision = SyncTaskPolicy::Decide(state, PolicyFor(state.kind), now, {});
        auto scheduled = decision.proposedState;
        scheduled.nextRunAt = decision.schedule.nextRunAt;
        if (!Persist(scheduled)) continue;
        state = scheduled;
        if (decision.schedule.shouldRunNow && CanStart(partition)) due.append(scheduled);
    }

    std::sort(due.begin(), due.end(), [](const SyncTaskState &left, const SyncTaskState &right) {
        if (left.priority != right.priority) return left.priority > right.priority;
        return ToString(left.partition) < ToString(right.partition);
    });
    if (!due.isEmpty()) StartTask(due.constFirst());
    ScheduleWakeUp();
}

void AdaptiveSyncCoordinator::Request(const SyncPartition partition, const SyncTaskTrigger trigger) {
    if (!started_ || stopped_) return;
    const auto iterator = states_.find(partition);
    if (iterator == states_.end()) return;
    if (activeGenerations_.contains(partition)) {
        rerunRequested_.insert(partition);
        return;
    }

    const auto decision = SyncTaskPolicy::Decide(iterator->second, PolicyFor(iterator->second.kind), Now(),
                                                  {.trigger = trigger});
    auto state = decision.proposedState;
    state.nextRunAt = decision.schedule.nextRunAt;
    if (state.partition != partition) {
        const auto existing = states_.find(state.partition);
        if (existing != states_.end()) {
            QString error;
            if (!stateRepository_.Remove(partition, error)) {
                emit SchedulingFailed(SafeError(error, QStringLiteral("Unable to retire promoted synchronization task state.")));
                return;
            }
            states_.erase(iterator);
            ScheduleWakeUp();
            return;
        }
        QString error;
        if (!stateRepository_.Remove(partition, error)) {
            emit SchedulingFailed(SafeError(error, QStringLiteral("Unable to promote synchronization task state.")));
            return;
        }
        if (!Persist(state)) return;
        states_.erase(iterator);
        states_[state.partition] = state;
    } else {
        if (!Persist(state)) return;
        iterator->second = state;
    }
    if (decision.schedule.shouldRunNow && activeGenerations_.empty() && CanStart(state.partition)) {
        StartTask(state);
    }
    ScheduleWakeUp();
}

void AdaptiveSyncCoordinator::StartTask(SyncTaskState state) {
    if (stopped_ || activeGenerations_.contains(state.partition) || !CanStart(state.partition)) return;

    if (state.status == SyncTaskStatus::Succeeded) {
        state.confirmedPage.reset();
        state.confirmedCursor.reset();
    }
    state.status = SyncTaskStatus::Running;
    state.lastAttemptedAt = Now();
    ++state.generation;
    if (!Persist(state)) return;
    states_[state.partition] = state;
    activeGenerations_[state.partition] = state.generation;
    emit TaskStarted(state.partition);

    QPointer<AdaptiveSyncCoordinator> coordinator(this);
    executor_.Execute(state, state.generation,
                      [coordinator, partition = state.partition, generation = state.generation](
                          const SyncTaskExecutionResult &result) {
        if (!coordinator) return;
        QMetaObject::invokeMethod(coordinator, [coordinator, partition, generation, result] {
            if (coordinator) coordinator->CompleteTask(partition, generation, result);
        }, Qt::QueuedConnection);
    });
}

void AdaptiveSyncCoordinator::CompleteTask(const SyncPartition partition, const qint64 generation,
                                           const SyncTaskExecutionResult result) {
    if (stopped_ || stopping_) return;
    const auto active = activeGenerations_.find(partition);
    const auto state = states_.find(partition);
    if (active == activeGenerations_.end() || state == states_.end() || active->second != generation
        || state->second.generation != generation) {
        return;
    }
    auto completed = state->second;
    if (result.succeeded) {
        completed.status = SyncTaskStatus::Succeeded;
        completed.lastSucceededAt = Now();
        completed.lastErrorCategory = AniListSyncErrorCategory::None;
        completed.safeErrorDetail.clear();
        completed.consecutiveFailures = 0;
        completed.consecutiveImmediateRetries = 0;
        if (partition == SyncPartition::UserList) userListReady_ = true;
        if (result.confirmedPage.has_value()) completed.confirmedPage = result.confirmedPage;
        if (result.confirmedCursor.has_value()) completed.confirmedCursor = result.confirmedCursor;
    } else {
        completed.lastErrorCategory = result.errorCategory == AniListSyncErrorCategory::None
            ? AniListSyncErrorCategory::Unknown : result.errorCategory;
        completed.safeErrorDetail = SafeError(result.safeErrorDetail,
                                               QStringLiteral("Synchronization task failed."));
        ++completed.consecutiveFailures;
        completed.status = SyncTaskStatus::RetryScheduled;
    }

    const auto decision = SyncTaskPolicy::Decide(completed, PolicyFor(completed.kind), Now(),
                                                  {.retryAfter = result.retryAfter});
    completed = decision.proposedState;
    completed.nextRunAt = decision.schedule.nextRunAt;
    if (!result.succeeded && decision.schedule.shouldRunNow) ++completed.consecutiveImmediateRetries;
    if (!Persist(completed)) return;
    activeGenerations_.erase(active);
    states_[partition] = completed;
    if (result.succeeded) {
        emit TaskCompleted(partition);
    } else {
        emit TaskFailed(partition, completed.safeErrorDetail);
    }
    if (result.succeeded && rerunRequested_.erase(partition) > 0) {
        Request(partition, SyncTaskTrigger::LocalChange);
    } else {
        rerunRequested_.erase(partition);
        ProcessDueTasks();
    }
}

void AdaptiveSyncCoordinator::ScheduleWakeUp() {
    if (stopped_ || !started_) return;

    std::optional<QDateTime> nearest;
    for (const auto &[partition, state] : states_) {
        if (activeGenerations_.contains(partition) || !state.nextRunAt.has_value()
            || !CanStart(partition)) {
            continue;
        }
        if (!nearest.has_value() || *state.nextRunAt < *nearest) nearest = state.nextRunAt;
    }
    if (!nearest.has_value()) {
        wakeUpTimer_->stop();
        return;
    }
    const auto delay = std::max<qint64>(0, Now().msecsTo(*nearest));
    wakeUpTimer_->start(static_cast<int>(std::min<qint64>(delay, std::numeric_limits<int>::max())));
}

bool AdaptiveSyncCoordinator::Persist(const SyncTaskState &state) {
    QString error;
    if (stateRepository_.Upsert(state, error)) return true;
    emit SchedulingFailed(SafeError(error, QStringLiteral("Unable to persist synchronization task state.")));
    return false;
}

void AdaptiveSyncCoordinator::AcknowledgeCancellation(const SyncPartition partition, const qint64 generation) {
    const auto active = activeGenerations_.find(partition);
    if (active != activeGenerations_.end() && active->second == generation) activeGenerations_.erase(active);
    FinishStopping();
}

void AdaptiveSyncCoordinator::AcknowledgeShutdown() {
    shutdownAcknowledged_ = true;
    FinishStopping();
}

void AdaptiveSyncCoordinator::FinishStopping() {
    if (!stopping_ || !shutdownAcknowledged_ || !activeGenerations_.empty()) return;
    stopped_ = true;
    stopping_ = false;
    emit Stopped();
}

QDateTime AdaptiveSyncCoordinator::Now() const {
    return (clock_ ? clock_() : QDateTime::currentDateTimeUtc()).toUTC();
}

const SyncSchedulePolicy &AdaptiveSyncCoordinator::PolicyFor(const SyncTaskKind kind) const {
    const auto policy = policies_.find(kind);
    if (policy != policies_.end()) return policy->second;
    static const auto defaultPolicies = DefaultSyncTaskPolicies();
    return defaultPolicies.at(kind);
}

bool AdaptiveSyncCoordinator::CanStart(const SyncPartition partition) const {
    return partition != SyncPartition::PendingChanges
        || userListReady_
        || !states_.contains(SyncPartition::UserList);
}
