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
}

AdaptiveSyncCoordinator::AdaptiveSyncCoordinator(
    ISyncTaskStateRepository &stateRepository, ISyncTaskExecutor &executor, Clock clock,
    std::map<SyncTaskKind, SyncSchedulePolicy> policies, QObject *parent)
    : QObject(parent), stateRepository_(stateRepository), executor_(executor), clock_(std::move(clock)),
      policies_(std::move(policies)), wakeUpTimer_(new QTimer(this)) {
    wakeUpTimer_->setSingleShot(true);
    connect(wakeUpTimer_, &QTimer::timeout, this, &AdaptiveSyncCoordinator::ProcessDueTasks);
}

AdaptiveSyncCoordinator::~AdaptiveSyncCoordinator() {
    Stop();
}

void AdaptiveSyncCoordinator::Start() {
    if (stopped_ || started_) return;

    QList<SyncTaskState> persistedStates;
    QString error;
    if (!stateRepository_.ReadAll(persistedStates, error)) {
        emit SchedulingFailed(SafeError(error, QStringLiteral("Unable to load synchronization task state.")));
        return;
    }

    for (const auto &state : persistedStates) states_[state.partition] = state;
    started_ = true;
    ProcessDueTasks();
}

void AdaptiveSyncCoordinator::Stop() {
    if (stopped_) return;

    stopped_ = true;
    wakeUpTimer_->stop();
    for (const auto &[partition, generation] : activeGenerations_) {
        executor_.Cancel(partition);
        auto state = states_.at(partition);
        state.generation = generation + 1;
        state.status = SyncTaskStatus::Idle;
        Persist(state);
    }
    activeGenerations_.clear();
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

    const auto now = Now();
    QList<SyncTaskState> due;
    for (auto &[partition, state] : states_) {
        if (activeGenerations_.contains(partition)) continue;
        const auto decision = SyncTaskPolicy::Decide(state, PolicyFor(state.kind), now, {});
        state = decision.proposedState;
        state.nextRunAt = decision.schedule.nextRunAt;
        Persist(state);
        if (decision.schedule.shouldRunNow) due.append(state);
    }

    std::sort(due.begin(), due.end(), [](const SyncTaskState &left, const SyncTaskState &right) {
        if (left.priority != right.priority) return left.priority > right.priority;
        return ToString(left.partition) < ToString(right.partition);
    });
    for (const auto &state : due) StartTask(state);
    ScheduleWakeUp();
}

void AdaptiveSyncCoordinator::Request(const SyncPartition partition, const SyncTaskTrigger trigger) {
    if (!started_ || stopped_) return;
    const auto iterator = states_.find(partition);
    if (iterator == states_.end() || activeGenerations_.contains(partition)) return;

    const auto decision = SyncTaskPolicy::Decide(iterator->second, PolicyFor(iterator->second.kind), Now(),
                                                  {.trigger = trigger});
    auto state = decision.proposedState;
    state.nextRunAt = decision.schedule.nextRunAt;
    if (state.partition != partition) {
        QString error;
        if (!stateRepository_.Remove(partition, error)) {
            emit SchedulingFailed(SafeError(error, QStringLiteral("Unable to promote synchronization task state.")));
            return;
        }
        states_.erase(iterator);
        states_[state.partition] = state;
    } else {
        iterator->second = state;
    }
    Persist(state);
    if (decision.schedule.shouldRunNow && !activeGenerations_.contains(state.partition)) StartTask(state);
    ScheduleWakeUp();
}

void AdaptiveSyncCoordinator::StartTask(SyncTaskState state) {
    if (stopped_ || activeGenerations_.contains(state.partition)) return;

    state.status = SyncTaskStatus::Running;
    state.lastAttemptedAt = Now();
    ++state.generation;
    states_[state.partition] = state;
    Persist(state);
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
    if (stopped_) return;
    const auto active = activeGenerations_.find(partition);
    const auto state = states_.find(partition);
    if (active == activeGenerations_.end() || state == states_.end() || active->second != generation
        || state->second.generation != generation) {
        return;
    }
    activeGenerations_.erase(active);

    auto completed = state->second;
    if (result.succeeded) {
        completed.status = SyncTaskStatus::Succeeded;
        completed.lastSucceededAt = Now();
        completed.lastErrorCategory = AniListSyncErrorCategory::None;
        completed.safeErrorDetail.clear();
        completed.consecutiveFailures = 0;
        completed.consecutiveImmediateRetries = 0;
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

    const auto decision = SyncTaskPolicy::Decide(completed, PolicyFor(completed.kind), Now(), {});
    completed = decision.proposedState;
    completed.nextRunAt = decision.schedule.nextRunAt;
    if (!result.succeeded && decision.schedule.shouldRunNow) ++completed.consecutiveImmediateRetries;
    states_[partition] = completed;
    Persist(completed);
    if (result.succeeded) {
        emit TaskCompleted(partition);
    } else {
        emit TaskFailed(partition, completed.safeErrorDetail);
    }
    ScheduleWakeUp();
}

void AdaptiveSyncCoordinator::ScheduleWakeUp() {
    if (stopped_ || !started_) return;

    std::optional<QDateTime> nearest;
    for (const auto &[partition, state] : states_) {
        if (activeGenerations_.contains(partition) || !state.nextRunAt.has_value()) continue;
        if (!nearest.has_value() || *state.nextRunAt < *nearest) nearest = state.nextRunAt;
    }
    if (!nearest.has_value()) {
        wakeUpTimer_->stop();
        return;
    }
    const auto delay = std::max<qint64>(0, Now().msecsTo(*nearest));
    wakeUpTimer_->start(static_cast<int>(std::min<qint64>(delay, std::numeric_limits<int>::max())));
}

void AdaptiveSyncCoordinator::Persist(const SyncTaskState &state) {
    QString error;
    if (!stateRepository_.Upsert(state, error)) {
        emit SchedulingFailed(SafeError(error, QStringLiteral("Unable to persist synchronization task state.")));
    }
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
