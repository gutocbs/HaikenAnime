#include "SyncTaskPolicy.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
std::chrono::milliseconds nonNegativeDuration(const std::chrono::milliseconds duration) {
    return std::max(duration, std::chrono::milliseconds::zero());
}

QDateTime clampedDueTime(QDateTime anchor, const std::chrono::milliseconds duration, const QDateTime now) {
    const auto delay = nonNegativeDuration(duration);
    if (!anchor.isValid() || anchor > now) anchor = now;
    const auto latest = now.addMSecs(delay.count());
    const auto due = anchor.addMSecs(delay.count());
    if (!due.isValid() || !latest.isValid() || due < now) return now;
    return std::min(due, latest);
}

std::chrono::milliseconds retryDelay(const SyncTaskState &state, const SyncSchedulePolicy &policy) {
    const int exponent = std::clamp(state.consecutiveFailures - 1, 0, 30);
    const auto initial = nonNegativeDuration(policy.initialRetryDelay).count();
    const auto maximum = nonNegativeDuration(policy.maximumRetryDelay);
    const auto multiplier = qint64{1} << exponent;
    const auto bounded = initial > std::numeric_limits<qint64>::max() / multiplier
        ? std::numeric_limits<qint64>::max() : initial * multiplier;
    return std::min(std::chrono::milliseconds(bounded), maximum);
}

CacheValidity cacheValidity(const SyncTaskState &state, const SyncSchedulePolicy &policy,
                            const QDateTime now) {
    if (!state.lastSucceededAt.has_value() || !state.lastSucceededAt->isValid()) return state.cacheValidity;
    const auto succeededAt = state.lastSucceededAt->toUTC();
    if (succeededAt.addMSecs(nonNegativeDuration(policy.normalInterval).count()) >= now) {
        return CacheValidity::Fresh;
    }
    if (succeededAt.addMSecs(nonNegativeDuration(policy.staleProtectionTtl).count()) >= now) {
        return CacheValidity::Stale;
    }
    return CacheValidity::Expired;
}

std::chrono::milliseconds withInjectedJitter(const std::chrono::milliseconds duration,
                                             const std::chrono::milliseconds requestedJitter,
                                             const double jitterRatio) {
    const auto safeDuration = nonNegativeDuration(duration);
    const auto safeRatio = std::clamp(jitterRatio, 0.0, 1.0);
    const auto maximumJitter = static_cast<qint64>(std::llround(safeDuration.count() * safeRatio));
    const auto jitter = std::clamp(requestedJitter.count(), -maximumJitter, maximumJitter);
    if (jitter < 0 && safeDuration.count() < -jitter) return std::chrono::milliseconds::zero();
    return safeDuration + std::chrono::milliseconds(jitter);
}

SyncScheduleDecision immediateDecision(const QDateTime now, const CacheValidity cacheValidity) {
    return {.shouldRunNow = true, .nextRunAt = now, .cacheValidity = cacheValidity};
}
}

int SyncTaskPolicy::PriorityFor(const SyncPartition partition) {
    switch (partition) {
    case SyncPartition::UserList: return 700;
    case SyncPartition::PendingChanges: return 600;
    case SyncPartition::ActiveCatalog: return 500;
    case SyncPartition::InactiveCatalog: return 300;
    case SyncPartition::CompletedCatalog: return 200;
    case SyncPartition::Covers: return 100;
    case SyncPartition::DerivedMetadata: return 50;
    }
    return 0;
}

SyncTaskPolicyDecision SyncTaskPolicy::Decide(const SyncTaskState &state, const SyncSchedulePolicy &policy,
                                              QDateTime now, const SyncTaskPolicyRequest &request) {
    now = now.toUTC();
    SyncTaskPolicyDecision decision;
    decision.proposedState = state;
    decision.schedule.cacheValidity = cacheValidity(state, policy, now);
    decision.proposedState.cacheValidity = decision.schedule.cacheValidity;

    if (state.status == SyncTaskStatus::Succeeded) {
        decision.resetFailureState = state.lastErrorCategory != AniListSyncErrorCategory::None
            || !state.safeErrorDetail.isEmpty() || state.consecutiveFailures != 0
            || state.consecutiveImmediateRetries != 0;
        decision.proposedState.lastErrorCategory = AniListSyncErrorCategory::None;
        decision.proposedState.safeErrorDetail.clear();
        decision.proposedState.consecutiveFailures = 0;
        decision.proposedState.consecutiveImmediateRetries = 0;
    }

    if (request.trigger == SyncTaskTrigger::CompletedToActive) {
        decision.proposedState.kind = SyncTaskKind::ActiveCatalog;
        decision.proposedState.partition = SyncPartition::ActiveCatalog;
        decision.proposedState.priority = PriorityFor(SyncPartition::ActiveCatalog);
        decision.promoted = true;
        decision.schedule = immediateDecision(now, decision.schedule.cacheValidity);
        return decision;
    }

    if (request.trigger == SyncTaskTrigger::ManualRun || request.trigger == SyncTaskTrigger::LocalChange) {
        decision.proposedState.priority = PriorityFor(decision.proposedState.partition);
        decision.promoted = request.trigger == SyncTaskTrigger::LocalChange;
        decision.schedule = immediateDecision(now, decision.schedule.cacheValidity);
        return decision;
    }

    if (request.retryAfter.has_value() && request.retryAfter->isValid() && *request.retryAfter > now) {
        decision.schedule.nextRunAt = request.retryAfter->toUTC();
        return decision;
    }

    const auto &effectiveState = decision.proposedState;
    const auto lastAttempted = effectiveState.lastAttemptedAt.value_or(now);
    if (effectiveState.lastErrorCategory != AniListSyncErrorCategory::None) {
        if (IsRetryable(effectiveState.lastErrorCategory)) {
            if (effectiveState.consecutiveImmediateRetries < policy.maximumConsecutiveImmediateRetries) {
                decision.schedule = immediateDecision(now, decision.schedule.cacheValidity);
                return decision;
            }
            const auto delay = std::min(withInjectedJitter(retryDelay(effectiveState, policy), request.jitter,
                                                            policy.jitterRatio),
                                        nonNegativeDuration(policy.maximumRetryDelay));
            decision.schedule.nextRunAt = clampedDueTime(lastAttempted, delay, now);
        } else {
            decision.schedule.nextRunAt = clampedDueTime(lastAttempted,
                                                          withInjectedJitter(policy.cooldown, request.jitter,
                                                                             policy.jitterRatio), now);
        }
        decision.schedule.shouldRunNow = decision.schedule.nextRunAt <= now;
        return decision;
    }

    if (decision.schedule.cacheValidity == CacheValidity::Fresh && effectiveState.lastSucceededAt.has_value()) {
        decision.schedule.nextRunAt = clampedDueTime(*effectiveState.lastSucceededAt,
                                                      withInjectedJitter(policy.normalInterval, request.jitter,
                                                                         policy.jitterRatio), now);
        decision.schedule.shouldRunNow = decision.schedule.nextRunAt <= now;
        return decision;
    }

    decision.schedule = immediateDecision(now, decision.schedule.cacheValidity);
    return decision;
}

SyncScheduleDecision SyncTaskPolicy::Evaluate(const SyncTaskState &state, const SyncSchedulePolicy &policy,
                                              QDateTime now, std::optional<QDateTime> retryAfter) {
    return Decide(state, policy, now, {.retryAfter = std::move(retryAfter)}).schedule;
}
